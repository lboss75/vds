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
  std::list<vds::ichunk_manager::object_chunk_map >& result)
{
  static_cast<_chunk_manager *>(this)->get_object_map(
    sp,
    tr,
    object_id,
    result);
}

void vds::ichunk_manager::query_object_chunk(
  const vds::service_provider& sp,
  vds::database_transaction& tr,
  const vds::guid & server_id,
  vds::ichunk_manager::index_type chunk_index,
  size_t & downloaded_data,
  size_t& total_data)
{
  static_cast<_chunk_manager *>(this)->query_object_chunk(
  sp,
  tr,
  server_id,
  chunk_index,
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

/*
vds::async_task<>
vds::_chunk_manager::add(
  const service_provider & sp,
  const guid & owner_principal,
  server_log_file_map & target,
  const filename & fn)
{
  return create_async_task(
    [this, fn, &target, owner_principal](
      const std::function<void (const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp){
        const std::shared_ptr<std::exception> & error;

        constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
        auto file_size = file::length(fn);
        for (decltype(file_size) offset = 0; offset < file_size; offset += BLOCK_SIZE) {

          this->tmp_folder_mutex_.lock();
          auto tmp_index = this->last_tmp_file_index_++;
          this->tmp_folder_mutex_.unlock();

          filename tmp_file(this->tmp_folder_, std::to_string(tmp_index));

          size_t original_lenght;
          const_data_buffer original_hash;

          dataflow(
            file_range_read(fn, offset, BLOCK_SIZE),
            hash_filter(&original_lenght, &original_hash),
            file_write(tmp_file, file::file_mode::create_new)
          )(
            [this, tmp_file, &original_lenght, &original_hash, &target, owner_principal](const service_provider & sp) {

              this->obj_folder_mutex_.lock();
              auto index = this->last_obj_file_index_++;
              this->obj_folder_mutex_.unlock();

              file::move(tmp_file,
                sp.get<ilocal_cache>()->get_object_filename(
                  sp, sp.get<istorage_log>()->current_server_id(), index));

              auto result = principal_log_new_object(
                index,
                original_lenght,
                original_hash,
                owner_principal);

              sp.get<istorage_log>()->add_to_local_log(sp, result.serialize());
              target.add(result);
          },
            [&error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              error = ex;
            },
            sp);
          if (error) {
            break;
          }
        };

        if (error) {
          on_error(sp, error);
        }
        else {
          done(sp);
        }
      });
}

void vds::_chunk_manager::generate_chunk(const service_provider & sp)
{
  throw std::runtime_error("Not implemented");
}

void vds::_chunk_manager::set_next_index(const service_provider & sp, uint64_t next_index)
{
  this->last_obj_file_index_ = next_index;
}
*/

void vds::_chunk_manager::start(const service_provider & sp)
{
  auto server_id = sp.get<istorage_log>()->current_server_id();
  (*sp.get<iserver_database>())->get_db()->sync_transaction(sp,
    [this, sp, server_id](database_transaction & t){

    this->last_chunk_ = this->get_last_chunk(
      sp,
      t,
      server_id);
    
    this->tail_chunk_index_ = this->get_tail_chunk(
      sp,
      t,
      server_id,
      this->tail_chunk_size_);

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
    principal_log_new_object_map result(server_id, version_id, file_size, file_hash, BLOCK_SIZE, REPLICA_SIZE);
        
    for (decltype(file_size) offset = 0; offset < file_size; offset += BLOCK_SIZE) {
      if(BLOCK_SIZE < file_size - offset){
        if (!this->write_chunk(sp, tr, result, tmp_file, offset, BLOCK_SIZE, on_error)) {
          return;
        }
      }
      else {
        if (!this->write_tail(sp, tr, result, tmp_file, offset, file_size - offset, on_error)) {
          return;
        }
      }
    }
    
    sp.get<logger>()->debug(sp, "Add object %s", result.serialize()->str().c_str());
    sp.get<istorage_log>()->add_to_local_log(
      sp,
      tr,
      server_id,
      sp.get<istorage_log>()->server_private_key(),
      result.serialize(),
      false,
      version_id);
    
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
      chunk_size INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk PRIMARY KEY (server_id, chunk_index))");

    t.execute(
      "CREATE TABLE object_chunk_map(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      object_id VARCHAR(64) NOT NULL,\
      object_offset INTEGER NOT NULL,\
      chunk_offset INTEGER NOT NULL,\
      length INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_map PRIMARY KEY (server_id, chunk_index, object_id))");

    t.execute(
      "CREATE TABLE tmp_object_chunk(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      CONSTRAINT pk_tmp_object_chunk PRIMARY KEY (server_id, chunk_index))");

    t.execute(
      "CREATE TABLE tmp_object_chunk_map(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      object_id VARCHAR(64) NOT NULL,\
      object_offset INTEGER NOT NULL,\
      chunk_offset INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      data BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_map PRIMARY KEY (server_id, chunk_index, object_id))");

    t.execute(
      "CREATE TABLE object_chunk_replica(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      replica_length INTEGER NOT NULL,\
      replica_hash VARCHAR(64) NOT NULL,\
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
  principal_log_new_object_map & result_record,
  const filename & fn,
  size_t offset,
  size_t size,
  const error_handler & on_error)
{
  this->chunk_mutex_.lock();
  auto index = this->last_chunk_++;
  this->chunk_mutex_.unlock();
  
  size_t original_length;
  const_data_buffer original_hash;

  bool result = true;
  std::vector<uint8_t> buffer;
  dataflow(
    file_range_read(fn, offset, size),
    hash_filter(&original_length, &original_hash),
    collect_data(buffer)
  )(
    [this, &tr, &result, &original_length, &original_hash, &buffer, index, &result_record, offset, size, on_error](const service_provider & sp){
      
      if (original_length != size) {
        on_error(sp, std::make_shared<std::runtime_error>("File is corrupt"));
        result = false;
        return;
      }

      chunk_info chunk(index, original_hash);      
      
      auto server_id = sp.get<istorage_log>()->current_server_id();
      this->add_full_chunk(
        sp,
        tr,
        result_record.object_id(),
        offset,
        size,
        original_hash,
        server_id,
        index);
      
      
      result = this->generate_horcruxes(
        sp,
        tr,
        result_record.server_id(),
        chunk,
        buffer,
        on_error);
      
      result_record.full_chunks().push_back(chunk);
    },
    [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
      on_error(sp, ex);
      result = false;
    },
    sp);
  
  return result;
}

bool vds::_chunk_manager::write_tail(
  const service_provider & sp,
  database_transaction & tr,
  principal_log_new_object_map & result_record,
  const filename & fn,
  size_t offset,
  size_t size,
  const error_handler & on_error)
{
  auto server_id = sp.get<istorage_log>()->current_server_id();
  bool result = true;
  while(0 < size) {
    std::lock_guard<std::mutex> lock(this->tail_chunk_mutex_);

    auto index = this->tail_chunk_index_;

    auto to_write = BLOCK_SIZE - this->tail_chunk_size_;
    if (to_write <= size) {
      size_t original_length;
      const_data_buffer original_hash;
      std::vector<uint8_t> tail_buffer;
      dataflow(
        file_range_read(fn, offset, to_write),
        hash_filter(&original_length, &original_hash),
        collect_data(tail_buffer))(
          [this, sp, &tr, server_id, &result_record, index, offset, to_write, &original_hash, &tail_buffer](const service_provider & sp) {
            this->add_to_tail_chunk(
              sp,
              tr,
              result_record.object_id(),
              offset,
              original_hash,
              server_id,
              index,
              this->tail_chunk_size_,
              tail_buffer);
            
            result_record.tail_chunk_items().push_back(
              tail_chunk_item(
                server_id,
                result_record.object_id(),
                index,
                offset,
                this->tail_chunk_size_,
                tail_buffer.size(),
                original_hash));
          },
          [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            on_error(sp, ex);
            result = false;
          },
          sp);
      if (!result) {
        break;
      }

      offset += to_write;
      size -= to_write;
      this->tail_chunk_size_ += to_write;

      if (BLOCK_SIZE == this->tail_chunk_size_) {
        this->chunk_mutex_.lock();
        this->tail_chunk_index_ = this->last_chunk_++;
        this->tail_chunk_size_ = 0;
        this->chunk_mutex_.unlock();

        this->start_tail_chunk(
          sp,
          tr,
          server_id,
          this->tail_chunk_index_);

        result = this->generate_tail_horcruxes(
          sp,
          tr,
          server_id,
          index,
          on_error);

        if (!result) {
          break;
        }
      }
    }
    else {
      size_t original_length;
      const_data_buffer original_hash;
      std::vector<uint8_t> tail_buffer;
      dataflow(
        file_range_read(fn, offset, size),
        hash_filter(&original_length, &original_hash),
        collect_data(tail_buffer))(
          [this, sp, &tr, server_id, &result_record, index, offset, size, &original_hash, &tail_buffer](const service_provider & sp) {
            this->add_to_tail_chunk(
              sp,
              tr,
              result_record.object_id(),
              offset,
              original_hash,
              server_id,
              index,
              this->tail_chunk_size_,
              tail_buffer);
            
            result_record.tail_chunk_items().push_back(
              tail_chunk_item(
                server_id,
                result_record.object_id(),
                index,
                offset,
                this->tail_chunk_size_,
                tail_buffer.size(),
                original_hash));
          },
          [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            on_error(sp, ex);
            result = false;
          },
          sp);
        
      if (result) {
        this->tail_chunk_size_ += size;
      }
      
      break;
    }
  }

  return result;
}

bool vds::_chunk_manager::generate_tail_horcruxes(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t chunk_index,
  const error_handler & on_error)
{
  auto data = this->get_tail_data(sp, tr, server_id, chunk_index);

  size_t original_length;
  const_data_buffer original_hash;

  bool result = true;
  std::vector<uint8_t> buffer;
  dataflow(
    dataflow_arguments<uint8_t>(data.data(), data.size()),  
    hash_filter(&original_length, &original_hash),
    collect_data(buffer)
  )(
    [this, &tr, &result, &original_length, &original_hash, &buffer, server_id, chunk_index, on_error](const service_provider & sp) {

      this->final_tail_chunk(
        sp,
        tr,
        original_length,
        original_hash,
        server_id,
        chunk_index);

      chunk_info chunk(chunk_index, original_hash);
      result = this->generate_horcruxes(
        sp,
        tr,
        server_id,
        chunk,
        buffer,
        on_error);
    },
    [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
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
  chunk_info & chunk_info,
  const std::vector<uint8_t> & buffer,
  const error_handler & on_error)
{
  bool result = true;

  for (uint16_t replica = 0; replica < GENERATE_HORCRUX; ++replica) {

    auto replica_data = this->chunk_storage_.generate_replica(replica, buffer.data(), buffer.size());
    assert(REPLICA_SIZE == replica_data.size());
    size_t replica_length;
    const_data_buffer replica_hash;
    std::vector<uint8_t> buffer;
    dataflow(
      dataflow_arguments<uint8_t>(replica_data.data(), replica_data.size()),
      hash_filter(&replica_length, &replica_hash),
      collect_data(buffer))(
      [this, &tr, server_id, &chunk_info, replica, &replica_length, &replica_hash, &buffer](const service_provider & sp) {
          this->add_chunk_replica(
            sp,
            tr,
            server_id,
            chunk_info.chunk_index(),
            replica,
            replica_length,
            replica_hash);
          chunk_info.add_replica_hash(replica_hash);

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

  size_t result = 0;
  while (reader.execute()) {
    reader.get_value(0, result);
  }

  return result;
}

size_t vds::_chunk_manager::get_tail_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t & result_size)
{
  tmp_object_chunk_table t;

  auto reader = tr.get_reader(
      t.select(t.chunk_index).where(t.server_id == server_id));

  size_t chunk_index = 0;
  while (reader.execute()) {
    chunk_index = t.chunk_index.get(reader);
  }

  tmp_object_chunk_map_table t1;
  reader = tr.get_reader(
      t1.select(db_sum(db_length(t1.chunk_index)))
      .where(t1.server_id == server_id && t1.chunk_index == chunk_index));

  while (reader.execute()) {
    reader.get_value(0, result_size);
  }

  return chunk_index;
}


void vds::_chunk_manager::add_full_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & object_id,
  size_t offset,
  size_t size,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index)
{
  this->add_chunk(sp, tr, server_id, index, size, object_hash);
  this->add_object_chunk_map(sp, tr, server_id, index, object_id, offset, 0, size, object_hash);
}

void vds::_chunk_manager::add_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t index,
  size_t size,
  const const_data_buffer & object_hash)
{
  object_chunk_table t;

  tr.execute(
    t.insert(t.server_id = server_id, t.chunk_index = index, t.chunk_size = size, t.hash = object_hash));
}

void vds::_chunk_manager::add_object_chunk_map(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t chunk_index,
  const guid & object_id,
  size_t object_offset,
  size_t chunk_offset,
  size_t length,
  const const_data_buffer & hash)
{
  object_chunk_map_table t;

  tr.execute(
    t.insert(
      t.server_id = server_id,
      t.chunk_index = chunk_index,
      t.object_id = object_id,
      t.object_offset = object_offset,
      t.chunk_offset = chunk_offset,
      t.length = length,
      t.hash = hash));
}

void vds::_chunk_manager::get_object_map(
  const service_provider & sp,
  database_transaction & tr,
  const guid & object_id,
  std::list<object_chunk_map> & result)
{
  object_chunk_map_table t;
  auto st = tr.get_reader(
    t.select(t.server_id,t.chunk_index,t.object_offset,t.chunk_offset,t.length,t.hash)
    .where(t.object_id == object_id));
  
  while (st.execute()) {
    object_chunk_map item;

    item.server_id = t.server_id.get(st);
    item.chunk_index = t.chunk_index.get(st);
    item.object_offset = t.object_offset.get(st);
    item.chunk_offset = t.chunk_offset.get(st);
    item.length = t.length.get(st);
    item.hash = t.hash.get(st);

    item.object_id = object_id;

    result.push_back(item);
  }
  if(result.empty()){
    throw std::runtime_error("Object " + object_id.str() + " not found");
  }
}

void vds::_chunk_manager::start_tail_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t chunk_index)
{
  tmp_object_chunk_table t;
  tr.execute(
    t.insert(t.server_id = server_id, t.chunk_index = chunk_index));
}

void vds::_chunk_manager::add_tail_object_chunk_map(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t chunk_index,
  const guid & object_id,
  size_t object_offset,
  size_t chunk_offset,
  const const_data_buffer & hash,
  const const_data_buffer & data)
{
  tmp_object_chunk_map_table t;
  
  tr.execute(
    t.insert(
      t.server_id = server_id,
      t.chunk_index = chunk_index,
      t.object_id = object_id,
      t.object_offset = object_offset,
      t.chunk_offset = chunk_offset,
      t.hash = hash,
      t.data = data));
}

void vds::_chunk_manager::final_tail_chunk(
  const service_provider & sp,
  database_transaction & tr,
  size_t chunk_length,
  const const_data_buffer & chunk_hash,
  const guid & server_id,
  size_t chunk_index)
{
  this->add_chunk(sp, tr, server_id, chunk_index, chunk_length, chunk_hash);

  object_chunk_map_table t;
  tmp_object_chunk_map_table t1;
  
  tr.execute(
    t.insert_into(t.server_id,t.chunk_index,t.object_id,t.object_offset,t.chunk_offset,t.length,t.hash)
    .from(t1, t1.server_id,t1.chunk_index,t1.object_id,t1.object_offset,t1.chunk_offset,t1.length,t1.hash)
    .where(t1.server_id == server_id && t1.chunk_index == chunk_index));

  
  tr.execute(
    t1.delete_if(t1.server_id == server_id && t1.chunk_index == chunk_index));
}

void vds::_chunk_manager::add_to_tail_chunk(
  const service_provider & sp,
  database_transaction & tr,
  const guid & object_id,
  size_t offset,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index,
  size_t chunk_offset,
  const const_data_buffer & data)
{
  this->add_tail_object_chunk_map(
    sp,
    tr,
    server_id,
    index,
    object_id,
    offset,
    chunk_offset,
    object_hash,
    data);
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
    t1.insert(
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

vds::const_data_buffer vds::_chunk_manager::get_tail_data(
  const service_provider & sp,
  database_transaction & tr,
  const guid & server_id,
  size_t chunk_index)
{
  std::vector<uint8_t> data;

  tmp_object_chunk_map_table t;
  auto reader = tr.get_reader(
    t.select(t.data,t.chunk_offset)
    .where(t.server_id == server_id && t.chunk_index == chunk_index)
    .order_by(t.chunk_offset));
  
  while(reader.execute()){
    auto item = t.data.get(reader);
    auto offset = t.chunk_offset.get(reader);

    if (offset != data.size()) {
      throw std::runtime_error("Database is corrupted");
    }

    data.insert(data.end(), item.data(), item.data() + item.size());
  }

  return const_data_buffer(data);
}

void vds::_chunk_manager::query_object_chunk(
  const vds::service_provider& sp,
  vds::database_transaction& tr,
  const vds::guid & server_id,
  vds::ichunk_manager::index_type chunk_index,
  size_t & downloaded_data,
  size_t& total_data)
{
  object_chunk_data_table t;
  auto st = tr.get_reader(
    t.select(t.replica)
    .where(t.server_id == server_id && t.chunk_index == chunk_index));
  
  std::list<ichunk_manager::replica_type> replicas;
  while(st.execute()){
    replicas.push_back(t.replica.get(st));
  }
  
  total_data += BLOCK_SIZE;
  if(MIN_HORCRUX <= replicas.size()){
    downloaded_data += BLOCK_SIZE;//Ok
    return;
  }
  
  downloaded_data += REPLICA_SIZE * replicas.size();
  
  std::set<guid> data_request;
  
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
    sp.get<logger>()->debug(sp, "Route: Query chunk %s:%d for %s",
      server_id.str().c_str(),
      chunk_index,
      current_server_id.str().c_str());
    object_request message(server_id, chunk_index, current_server_id, replicas);
    connection_manager->send_to(sp, p, message);
  }
}