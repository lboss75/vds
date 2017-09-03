/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <unordered_set>
#include "chunk_manager.h"
#include "chunk_manager_p.h"
#include "storage_log.h"
#include "hash.h"
#include "server_database_p.h"
#include "object_transfer_protocol_p.h"
#include "connection_manager.h"

vds::ichunk_manager::ichunk_manager()
{
}

vds::ichunk_manager::~ichunk_manager()
{
}

void vds::ichunk_manager::start(const service_provider & sp)
{
  static_cast<_chunk_manager *>(this)->start(sp);
}

void vds::ichunk_manager::stop(const service_provider & sp)
{
  static_cast<_chunk_manager *>(this)->stop(sp);
}

vds::async_task<> vds::ichunk_manager::add_object(
  const service_provider & sp,
  database_transaction & tr,
  const guid & version_id,
  const filename & tmp_file,
  const const_data_buffer & file_hash)
{
  return static_cast<_chunk_manager *>(this)->add_object(
    sp,
    tr,
    version_id,
    tmp_file,
    file_hash);
}

void vds::ichunk_manager::get_object_map(
  const vds::service_provider& sp,
  vds::database_transaction & tr,
  const vds::guid & object_id,
  guid & server_id,
  ichunk_manager::index_type & min_chunk_index,
  ichunk_manager::index_type & max_chunk_index)
{
  static_cast<_chunk_manager *>(this)->get_object_map(
    sp,
    tr,
    object_id,
    server_id,
    min_chunk_index,
    max_chunk_index);
}

void vds::ichunk_manager::query_object_chunk(
  const vds::service_provider& sp,
  vds::database_transaction& tr,
  const vds::guid & server_id,
  vds::ichunk_manager::index_type chunk_index,
  const vds::guid & object_id,
  size_t & downloaded_data,
  size_t& total_data)
{
  static_cast<_chunk_manager *>(this)->query_object_chunk(
  sp,
  tr,
  server_id,
  chunk_index,
  object_id,
  downloaded_data,
  total_data);
}

/*
vds::async_task<>
vds::ichunk_manager::add(
  const service_provider & sp,
  const guid & owner_principal,
  server_log_file_map & target,
  const filename & fn)
{
  return static_cast<_chunk_manager *>(this)->add(sp, owner_principal, target, fn);
}

vds::const_data_buffer vds::ichunk_manager::get(
  const service_provider & sp,
  const guid & server_id, uint64_t index)
{
  return static_cast<_chunk_manager *>(this)->get(sp, server_id, index);
}

void vds::ichunk_manager::set_next_index(
  const service_provider & sp,
  uint64_t next_index)
{
  static_cast<_chunk_manager *>(this)->set_next_index(sp, next_index);
}
*/
//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager()
: chunk_storage_(MIN_HORCRUX)
{
}

vds::_chunk_manager::~_chunk_manager()
{
}

void vds::_chunk_manager::start(const service_provider & sp)
{
  auto server_id = sp.get<istorage_log>()->current_server_id();
  (*sp.get<iserver_database>())->get_db()->sync_transaction(sp,
    [this, sp, server_id](database_transaction & t){

    this->last_chunk_ = this->get_last_chunk(
      sp,
      t,
      server_id) + 1;
    
    return true;
  });
}

void vds::_chunk_manager::stop(const service_provider & sp)
{
}

vds::async_task<> vds::_chunk_manager::add_object(
  const service_provider & sp,
  database_transaction & tr,
  const guid & version_id,
  const filename & tmp_file,
  const const_data_buffer & file_hash)
{
  return create_async_task([this, &tr, version_id, tmp_file, file_hash](
    const std::function<void (const service_provider & sp)> & done,
    const error_handler & on_error,
    const service_provider & sp){
    
    auto file_size = file::length(tmp_file);
    auto server_id = sp.get<istorage_log>()->current_server_id();
    
    this->chunk_mutex_.lock();
    auto start_chunk = this->last_chunk_;
    auto finish_chunk = start_chunk + (file_size / BLOCK_SIZE);
    if(0 != (file_size % BLOCK_SIZE)){
      ++finish_chunk;
    }
    this->chunk_mutex_.unlock();
    
    //principal_log_new_object_map result(server_id, version_id, file_size, file_hash, start_chunk, finish_chunk);
        
    for (decltype(file_size) offset = 0; offset < file_size; offset += BLOCK_SIZE) {
      if (!this->write_chunk(
        sp,
        tr,
        start_chunk,
        version_id,
        tmp_file,
        offset,
        (BLOCK_SIZE < file_size - offset) ? BLOCK_SIZE : (file_size - offset),
        on_error,
        start_chunk == finish_chunk - 1)) {
        return;
      }
      
      ++start_chunk;
    }
    
    if(start_chunk != finish_chunk){
      throw std::runtime_error("Login error");
    }
    
    
    done(sp);
  });
}

void vds::_chunk_manager::create_database_objects(
  const service_provider & sp,
  uint64_t db_version,
  database_transaction & t)
{
  if (1 > db_version) {
    t.execute(
      "CREATE TABLE object_chunk(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      object_id VARCHAR(64) NOT NULL,\
      chunk_size INTEGER NOT NULL,\
      chunk_hash BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk PRIMARY KEY (server_id, chunk_index))");

    t.execute(
      "CREATE INDEX pk_object_chunk_object_id ON object_chunk(object_id)");

    t.execute(
      "CREATE TABLE object_chunk_replica(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      replica_length INTEGER NOT NULL,\
      replica_hash BLOB NOT NULL,\
      CONSTRAINT pk_tmp_object_chunk_map PRIMARY KEY (server_id, chunk_index, replica))");

    t.execute(
      "CREATE TABLE object_chunk_store(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      storage_id VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_object_chunk_store PRIMARY KEY (server_id, chunk_index, replica, storage_id))");

    t.execute(
      "CREATE TABLE object_chunk_data(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      data BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_data PRIMARY KEY (server_id, chunk_index, replica))");
  }
}

bool vds::_chunk_manager::write_chunk(
  const service_provider & sp,
  database_transaction & tr,
  size_t index,
  //principal_log_new_object_map & result_record,
  const guid & object_id,
  const filename & fn,
  size_t offset,
  size_t size,
  const error_handler & on_error,
  bool is_last)
{
  size_t original_length;
  const_data_buffer original_hash;

  bool result = true;
  std::vector<uint8_t> buffer;
  dataflow(
    file_range_read(fn, offset, size),
    hash_filter(&original_length, &original_hash),
    collect_data(buffer)
  )(
    [this, &tr, &result, &original_length, &original_hash, &buffer, index, object_id, offset, size, on_error, is_last](const service_provider & sp){
      
      if (original_length != size) {
        on_error(sp, std::make_shared<std::runtime_error>("File is corrupt"));
        result = false;
        return;
      }

      auto server_id = sp.get<istorage_log>()->current_server_id();
      this->add_chunk(
        sp,
        tr,
        server_id,
        index,
        object_id,
        original_length,
        original_hash);
      
      //Padding
      while(buffer.size() % (2 * MIN_HORCRUX) > 0){
        buffer.push_back(0);
      }
      
      principal_log_new_chunk chunk(server_id, index, object_id, original_length, original_hash);      
      result = this->generate_horcruxes(
        sp,
        tr,
        server_id,
        chunk,
        buffer,
        on_error);
      
    sp.get<istorage_log>()->add_to_local_log(
      sp,
      tr,
      server_id,
      sp.get<istorage_log>()->server_private_key(),
      chunk.serialize(true),
      false,
      is_last ? object_id : guid::new_guid());
    },
    [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
      on_error(sp, ex);
      result = false;
    },
    sp);
  
  return result;
}


bool vds::_chunk_manager::generate_horcruxes(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  principal_log_new_chunk & chunk_info,
  const std::vector<uint8_t> & buffer,
  const error_handler & on_error)
{
  bool result = true;

  for (uint16_t replica = 0; replica < GENERATE_HORCRUX; ++replica) {

    auto replica_data = this->chunk_storage_.generate_replica(replica, buffer.data(), buffer.size());
    size_t replica_length;
    const_data_buffer replica_hash;
    std::vector<uint8_t> buffer;
    dataflow(
      dataflow_arguments<uint8_t>(replica_data.data(), replica_data.size()),
      hash_filter(&replica_length, &replica_hash),
      collect_data(buffer))(
      [this, &tr, server_id, &chunk_info, replica, &replica_length, &replica_hash, &buffer](const service_provider & sp) {
        if(replica_length != chunk_info.replica_size()){
          throw std::runtime_error("Login error");
        }
          this->add_chunk_replica(
            sp,
            tr,
            server_id,
            chunk_info.chunk_index(),
            replica,
            replica_length,
            replica_hash);
          
          sp.get<istorage_log>()->add_to_local_log(
            sp,
            tr,
            server_id,
            sp.get<istorage_log>()->server_private_key(),
            principal_log_new_replica(
              server_id,
              chunk_info.chunk_index(),
              chunk_info.object_id(),
              replica,
              replica_length,
              replica_hash).serialize(true),
            false);
          

          this->add_chunk_store_data(
            sp,
            tr,
            server_id,
            chunk_info.chunk_index(),
            replica,
            sp.get<istorage_log>()->current_server_id(),
            buffer);
        },
        [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          on_error(sp, ex);
          result = false;
        },
        sp);
    if (!result) {
      break;
    }
  }

  return result;
}


size_t vds::_chunk_manager::get_last_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id)
{
  object_chunk_table t;

  auto reader = tr.get_reader(
      t.select(db_max(t.chunk_index))
      .where(t.server_id == server_id));

  size_t result = 1;
  while (reader.execute()) {
    reader.get_value(0, result);
  }

  return result;
}

void vds::_chunk_manager::add_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  ichunk_manager::index_type chunk_index,
  const guid & object_id,
  size_t chunk_size,
  const const_data_buffer & chunk_hash)
{
  object_chunk_table t;

  tr.execute(
    t.insert(
      t.server_id = server_id,
      t.chunk_index = chunk_index,
      t.object_id = object_id,
      t.chunk_size = chunk_size,
      t.chunk_hash = chunk_hash));
}


void vds::_chunk_manager::get_object_map(
  const service_provider & sp,
  database_transaction & tr,
  const guid & object_id,
  guid & server_id,
  ichunk_manager::index_type & min_chunk_index,
  ichunk_manager::index_type & max_chunk_index)
{
  object_chunk_table t;
  auto st = tr.get_reader(
    t.select(t.server_id, db_min(t.chunk_index), db_max(t.chunk_index))
    .where(t.object_id == object_id));
  
  if (st.execute()) {
    server_id = t.server_id.get(st);
    st.get_value(1, min_chunk_index);
    st.get_value(2, max_chunk_index);
    return;    
  }
  throw std::runtime_error("Object " + object_id.str() + " not found");
}


void vds::_chunk_manager::add_chunk_replica(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t index,
  uint16_t replica,
  size_t replica_length,
  const const_data_buffer & replica_hash)
{
  object_chunk_replica_table t;
  
  tr.execute(
    t.insert(
      t.server_id = server_id,
      t.chunk_index = index,
      t.replica = replica,
      t.replica_length = replica_length,
      t.replica_hash = replica_hash));
}

void vds::_chunk_manager::add_chunk_store(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  ichunk_manager::index_type index,
  uint16_t replica,
  const guid & storage_id)
{
  object_chunk_store_table t1;
  tr.execute(
    t1.insert_or_ignore(
      t1.server_id = server_id,
      t1.chunk_index = index,
      t1.replica = replica,
      t1.storage_id = storage_id));
}

void vds::_chunk_manager::add_chunk_store_data(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  ichunk_manager::index_type index,
  uint16_t replica,
  const guid & storage_id,
  const const_data_buffer & data)
{
  object_chunk_data_table t;
  tr.execute(
    t.insert_or_ignore(
      t.server_id = server_id,
      t.chunk_index = index,
      t.replica = replica,
      t.data = data));
  
  this->add_chunk_store(sp, tr, server_id, index, replica, storage_id);
}

void vds::_chunk_manager::get_replicas(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  ichunk_manager::index_type index,
  const guid & storage_id,
  std::list<ichunk_manager::replica_type>& result)
{
  object_chunk_store_table t;

  auto reader = tr.get_reader(
    t.select(t.replica).where(t.server_id == server_id && t.chunk_index == index && t.storage_id == storage_id));
  while (reader.execute()) {
    result.push_back((ichunk_manager::replica_type)t.replica.get(reader));
  }

  object_chunk_store_table t1;
  reader = tr.get_reader(
    t1.select(t1.replica).where(t1.server_id == server_id && t1.chunk_index == index && t1.storage_id == storage_id));
  while (reader.execute()) {
    result.push_back((ichunk_manager::replica_type)t1.replica.get(reader));
  }
}

vds::const_data_buffer vds::_chunk_manager::get_replica_data(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  ichunk_manager::index_type index,
  ichunk_manager::replica_type replica)
{
  vds::const_data_buffer result;
  
  object_chunk_data_table t;
  
  auto reader = tr.get_reader(
    t.select(t.data).where(
      t.server_id == server_id
      && t.chunk_index == index
      && t.replica == replica));
  
  if(reader.execute()){  
    result = t.data.get(reader);
  } else {
    throw std::runtime_error("Data not found");
  }

  return result;
}

void vds::_chunk_manager::get_chunk_store(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  index_type index,
  std::list<chunk_store>& result)
{

}

void vds::_chunk_manager::query_object_chunk(
  const vds::service_provider& sp,
  vds::database_transaction& tr,
  const vds::guid & server_id,
  vds::ichunk_manager::index_type chunk_index,
  const guid & object_id,
  size_t & downloaded_data,
  size_t & total_data)
{
  size_t chunk_size;
  
  object_chunk_table t2;
  auto st = tr.get_reader(
    t2.select(t2.chunk_size).where(t2.server_id == server_id && t2.chunk_index == chunk_index)
  );
  if(st.execute()){
    chunk_size = t2.chunk_size.get(st);
  }
  else {
    throw std::runtime_error("Chunk " + server_id.str() + ":" + std::to_string(chunk_index) + " not found");
  }
  
  object_chunk_data_table t;
  st = tr.get_reader(
    t.select(t.replica, db_length(t.data))
    .where(t.server_id == server_id && t.chunk_index == chunk_index));
  
  size_t replicas_size = 0;
  std::list<ichunk_manager::replica_type> replicas;
  while(st.execute()){
    replicas.push_back(t.replica.get(st));
    size_t replica_size;
    st.get_value(1, replica_size);
    replicas_size += replica_size;
  }

  total_data += chunk_size;
  if (MIN_HORCRUX <= replicas.size()) {
    downloaded_data += chunk_size;//Ok
    return;
  }

  downloaded_data += replicas_size;

  std::set<guid> data_request;
  data_request.emplace(server_id);
  
  object_chunk_store_table t1;
  st = tr.get_reader(
    t1.select(t1.replica, t1.storage_id)
    .where(t1.server_id == server_id && t1.chunk_index == chunk_index));
  while(st.execute()){
    auto storage_id = t1.storage_id.get(st);
    auto replica = t1.replica.get(st);
    if(replicas.end() == std::find(replicas.begin(), replicas.end(), replica)
      && data_request.end() == data_request.find(storage_id)){
      data_request.emplace(storage_id);
    }
  }
  
  auto current_server_id = sp.get<istorage_log>()->current_server_id();
  auto connection_manager = sp.get<iconnection_manager>();
  for(auto & p : data_request){
    sp.get<logger>()->debug(
      "route",
      sp, "Route: Query chunk %s:%d for %s",
      server_id.str().c_str(),
      chunk_index,
      current_server_id.str().c_str());
    object_request message(server_id, chunk_index, current_server_id, replicas);
    connection_manager->send_to(sp, p, message);
  }

}

vds::const_data_buffer vds::_chunk_manager::restore_object_chunk(
  const vds::service_provider & sp,
  vds::database_transaction & tr,
  const vds::guid & server_id,
  vds::ichunk_manager::index_type chunk_index,
  const guid & object_id,
  size_t & chunk_size,
  const_data_buffer & chunk_hash)
{
  object_chunk_data_table t;
  auto st = tr.get_reader(
    t.select(t.replica, t.data)
    .where(t.server_id == server_id && t.chunk_index == chunk_index));

  std::unordered_map<uint16_t, const_data_buffer> horcruxes;
  while (st.execute() && MIN_HORCRUX > horcruxes.size()) {
    horcruxes[t.replica.get(st)] = t.data.get(st);
  }

  if (MIN_HORCRUX > horcruxes.size()) {
    throw std::runtime_error("Logic error");
  }

  auto result = this->chunk_storage_.restore_data(horcruxes);
  
  object_chunk_table t1;
  st = tr.get_reader(
    t1.select(t1.chunk_size, t1.chunk_hash)
    .where(t1.server_id == server_id && t1.chunk_index == chunk_index));

  if(st.execute()) {
    chunk_size = t1.chunk_size.get(st);
    chunk_hash = t1.chunk_hash.get(st);
  }
  else {
    throw std::runtime_error("Login error");
  }

  return result;
}
