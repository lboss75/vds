/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_database.h"
#include "server_database_p.h"
#include "principal_record.h"
#include "storage_log.h"

void vds::iserver_database::add_principal(
  const service_provider & sp,
  const principal_record & record)
{
  static_cast<_server_database *>(this)->add_principal(sp, record);
}

vds::guid vds::iserver_database::get_root_principal(const service_provider & sp)
{
  return static_cast<_server_database *>(this)->get_root_principal(sp);
}

void vds::iserver_database::add_user_principal(
  const service_provider & sp,
  const std::string & login,
  const principal_record & record)
{
  static_cast<_server_database *>(this)->add_user_principal(sp, login, record);
}


std::unique_ptr<vds::principal_record> vds::iserver_database::find_principal(
  const service_provider & sp,
  const guid & object_name)
{
  return static_cast<_server_database *>(this)->find_principal(
    sp,
    object_name);
}

std::unique_ptr<vds::principal_record> vds::iserver_database::find_user_principal(
  const service_provider & sp,
  const std::string & object_name)
{
  return static_cast<_server_database *>(this)->find_user_principal(
    sp,
    object_name);
}

void vds::iserver_database::add_object(
  const service_provider & sp,
  const principal_log_new_object & index)
{
  static_cast<_server_database *>(this)->add_object(sp, index);
}

void vds::iserver_database::add_endpoint(
  const service_provider & sp,
  const std::string& endpoint_id,
  const std::string& addresses)
{
  static_cast<_server_database *>(this)->add_endpoint(sp, endpoint_id, addresses);
}

void vds::iserver_database::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string>& addresses)
{
  static_cast<_server_database *>(this)->get_endpoints(addresses);
}

vds::principal_log_record vds::iserver_database::add_local_record(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  const guid & principal_id,
  const std::shared_ptr<json_value> & message,
  const vds::asymmetric_private_key & principal_private_key,
  const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->add_local_record(
    sp,
    record_id,
    principal_id,
    message,
    principal_private_key,
    signature);
}

size_t vds::iserver_database::get_current_state(
  const service_provider & sp, 
  std::list<guid> & active_records)
{
  return static_cast<_server_database *>(this)->get_current_state(sp, active_records);
}

bool vds::iserver_database::save_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->save_record(sp, record, signature);
}

void vds::iserver_database::get_unknown_records(
  const service_provider & sp,
  std::list<principal_log_record::record_id>& result)
{
  static_cast<_server_database *>(this)->get_unknown_records(sp, result);
}

bool vds::iserver_database::get_record(
  const service_provider & sp,
  const principal_log_record::record_id & id,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_record(sp, id, result_record, result_signature);
}

bool vds::iserver_database::get_front_record(
  const service_provider & sp,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_front_record(sp, result_record, result_signature);
}

void vds::iserver_database::processed_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->processed_record(sp, id);
}

void vds::iserver_database::delete_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->delete_record(sp, id);
}

vds::iserver_database::principal_log_state vds::iserver_database::get_record_state(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  return static_cast<_server_database *>(this)->principal_log_get_state(sp, id);
}

size_t vds::iserver_database::get_last_chunk(
  const service_provider & sp,
  const guid & server_id)
{
  return static_cast<_server_database *>(this)->get_last_chunk(sp, server_id);
}

size_t vds::iserver_database::get_tail_chunk(
  const service_provider & sp,
  const guid & server_id,
  size_t & result_size)
{
  return static_cast<_server_database *>(this)->get_tail_chunk(
    sp,
    server_id,
    result_size);
}

void vds::iserver_database::add_full_chunk(
  const service_provider & sp,
  const guid & object_id,
  size_t offset,
  size_t size,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index)
{
  static_cast<_server_database *>(this)->add_full_chunk(
    sp,
    object_id,
    offset,
    size,
    object_hash,
    server_id,
    index);
}

void vds::iserver_database::start_tail_chunk(const service_provider & sp, const guid & server_id, size_t chunk_index)
{
  static_cast<_server_database *>(this)->start_tail_chunk(
    sp,
    server_id,
    chunk_index);
}

void vds::iserver_database::final_tail_chunk(
  const service_provider & sp,
  size_t chunk_length,
  const const_data_buffer & chunk_hash,
  const guid & server_id,
  size_t chunk_index)
{
  static_cast<_server_database *>(this)->final_tail_chunk(
    sp,
    chunk_length,
    chunk_hash,
    server_id,
    chunk_index);
}

void vds::iserver_database::add_to_tail_chunk(
  const service_provider & sp,
  const guid & object_id,
  size_t offset,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index,
  size_t chunk_offset,
  const const_data_buffer & data)
{
  static_cast<_server_database *>(this)->add_to_tail_chunk(
    sp,
    object_id,
    offset,
    object_hash,
    server_id,
    index,
    chunk_offset,
    data);
}

void vds::iserver_database::add_chunk_replica(
  const service_provider & sp,
  const guid & server_id,
  size_t index,
  uint16_t replica,
  size_t replica_length,
  const const_data_buffer & replica_hash)
{
  static_cast<_server_database *>(this)->add_chunk_replica(
    sp,
    server_id,
    index,
    replica,
    replica_length,
    replica_hash);
}

void vds::iserver_database::add_chunk_store(
  const service_provider & sp,
  const guid & server_id,
  size_t index,
  uint16_t replica,
  const guid & storage_id,
  const const_data_buffer & data)
{
  static_cast<_server_database *>(this)->add_chunk_store(
    sp,
    server_id,
    index,
    replica,
    storage_id,
    data);
}

vds::const_data_buffer vds::iserver_database::get_tail_data(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index)
{
  return static_cast<_server_database *>(this)->get_tail_data(
    sp,
    server_id,
    chunk_index);
}
////////////////////////////////////////////////////////
vds::_server_database::_server_database()
{
}

vds::_server_database::~_server_database()
{
}

void vds::_server_database::start(const service_provider & sp)
{
  //this->db_.start(sp);
  uint64_t db_version;

  filename db_filename(foldername(persistence::current_user(sp), ".vds"), "local.db");

  if (!file::exists(db_filename)) {
    db_version = 0;
    this->db_.open(db_filename);
  }
  else {
    this->db_.open(db_filename);
    
    auto st = this->db_.parse("SELECT version FROM module WHERE id='kernel'");
    if(!st.execute()){
      throw std::runtime_error("Database has been corrupted");
    }
    
    st.get_value(0, db_version);
  }


  if (1 > db_version) {
    this->db_.execute("CREATE TABLE module(\
    id VARCHAR(64) PRIMARY KEY NOT NULL,\
    version INTEGER NOT NULL,\
    installed DATETIME NOT NULL)");

    this->db_.execute(
      "CREATE TABLE principal(\
      id VARCHAR(64) NOT NULL,\
      cert TEXT NOT NULL,\
      key TEXT NOT NULL,\
      password_hash BLOB NOT NULL,\
      parent VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_principal PRIMARY KEY(id))");

    this->db_.execute(
      "CREATE TABLE user_principal(\
      id VARCHAR(64) NOT NULL,\
      login VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_user_principal PRIMARY KEY(id))");

    this->db_.execute(
      "CREATE TABLE object(\
      object_id VARCHAR(64) NOT NULL,\
      length INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      CONSTRAINT pk_objects PRIMARY KEY (object_id))");

    this->db_.execute(
      "CREATE TABLE endpoint(\
      endpoint_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      addresses TEXT NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE principal_log(\
      id VARCHAR(64) NOT NULL,\
      principal_id VARCHAR(64) NOT NULL,\
      message TEXT NOT NULL,\
      signature BLOB NOT NULL,\
      order_num INTEGER NOT NULL,\
      state INTEGER NOT NULL,\
      CONSTRAINT pk_principal_log PRIMARY KEY(id))");

    this->db_.execute(
      "CREATE TABLE principal_log_link(\
      parent_id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_principal_log_link PRIMARY KEY(parent_id,follower_id))");

    //Replicas stored on the server
    this->db_.execute(
      "CREATE TABLE object_chunk(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      chunk_size INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk PRIMARY KEY (server_id, chunk_index))");

    this->db_.execute(
      "CREATE TABLE object_chunk_map(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      object_id VARCHAR(64) NOT NULL,\
      object_offset INTEGER NOT NULL,\
      chunk_offset INTEGER NOT NULL,\
      length INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_map PRIMARY KEY (server_id, chunk_index, object_id))");

    this->db_.execute(
      "CREATE TABLE tmp_object_chunk(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      CONSTRAINT pk_tmp_object_chunk PRIMARY KEY (server_id, chunk_index))");

    this->db_.execute(
      "CREATE TABLE tmp_object_chunk_map(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      object_id VARCHAR(64) NOT NULL,\
      object_offset INTEGER NOT NULL,\
      chunk_offset INTEGER NOT NULL,\
      hash BLOB NOT NULL,\
      data BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_map PRIMARY KEY (server_id, chunk_index, object_id))");

    this->db_.execute(
      "CREATE TABLE object_chunk_replica(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      replica_length INTEGER NOT NULL,\
      replica_hash VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_tmp_object_chunk_map PRIMARY KEY (server_id, chunk_index, replica))");

    this->db_.execute(
      "CREATE TABLE object_chunk_store(\
      server_id VARCHAR(64) NOT NULL,\
      chunk_index INTEGER NOT NULL,\
      replica INTEGER NOT NULL,\
      storage_id VARCHAR(64) NOT NULL,\
      data BLOB NOT NULL,\
      CONSTRAINT pk_object_chunk_store PRIMARY KEY (server_id, chunk_index, replica, storage_id))");

    this->db_.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
    this->db_.execute("INSERT INTO endpoint(endpoint_id, addresses) VALUES('default', 'udp://127.0.0.1:8050;https://127.0.0.1:8050')");
  }
}

void vds::_server_database::stop(const service_provider & sp)
{
  this->db_.close();
}

void vds::_server_database::add_principal(
  const service_provider & sp,
  const principal_record & record)
{
  this->add_principal_statement_.execute(
    this->db_,
    "INSERT INTO principal(id, cert, key, password_hash, parent)\
      VALUES (@id, @cert, @key, @password_hash, @parent)",

    record.id(),
    record.cert_body(),
    record.cert_key(),
    record.password_hash(),
    record.parent_principal());
}

void vds::_server_database::add_user_principal(
  const service_provider & sp,
  const std::string & login,
  const principal_record & record)
{
  this->add_principal(sp, record);

  this->add_user_principal_statement_.execute(
    this->db_,
    "INSERT INTO user_principal(id, login)\
      VALUES (@id, @login)",

    record.id(),
    login);
}

vds::guid vds::_server_database::get_root_principal(
  const service_provider & sp)
{
  vds::guid result;

  auto st = this->db_.parse("SELECT id FROM principal WHERE id=parent");
  while(st.execute()) {
    if (0 < result.size()) {
      throw std::runtime_error("Database is corrupt");
    }
    st.get_value(0, result);
  }

  if (0 == result.size()) {
    throw std::runtime_error("Database is corrupt");
  }

  return result;
}

std::unique_ptr<vds::principal_record> vds::_server_database::find_principal(
  const service_provider & sp,
  const guid & object_name)
{
  std::unique_ptr<principal_record> result;
  this->find_principal_query_.query(
    this->db_,
    "SELECT parent,cert,key,password_hash\
     FROM principal\
     WHERE id=@id",
    [&result, object_name](sql_statement & st)->bool{
      
      guid parent;
      std::string body;
      std::string key;
      const_data_buffer password_hash;
      
      st.get_value(0, parent);
      st.get_value(1, body);
      st.get_value(2, key);
      st.get_value(3, password_hash);

      result.reset(new principal_record(
        parent,
        object_name,
        body,
        key,
        password_hash));
      
      return false; },
    object_name);
  
  return result;
}

std::unique_ptr<vds::principal_record> vds::_server_database::find_user_principal(
  const service_provider & sp,
  const std::string & object_name)
{
  std::unique_ptr<principal_record> result;
  this->find_user_principal_query_.query(
    this->db_,
    "SELECT p.parent,p.id,p.cert,p.key,p.password_hash\
     FROM principal p\
     INNER JOIN user_principal u\
     ON u.id=p.id\
     WHERE u.login=@login",
    [&result, object_name](sql_statement & st)->bool {

    guid parent;
    guid name;
    std::string body;
    std::string key;
    const_data_buffer password_hash;

    st.get_value(0, parent);
    st.get_value(1, name);
    st.get_value(2, body);
    st.get_value(3, key);
    st.get_value(4, password_hash);

    result.reset(new principal_record(
      parent,
      name,
      body,
      key,
      password_hash));

    return false; },
    object_name);

  return result;
}

void vds::_server_database::add_object(
  const service_provider & sp,
  const principal_log_new_object & index)
{
  this->add_object_statement_.execute(
    this->db_,
    "INSERT INTO object(object_id, lenght, hash)\
    VALUES (@object_id, @lenght, @hash)",
    index.index(),
    index.lenght(),
    index.hash());
}


void vds::_server_database::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  this->add_endpoint_statement_.execute(
    this->db_,
    "INSERT INTO endpoint(endpoint_id, addresses) VALUES (@endpoint_id, @addresses)",
    endpoint_id,
    addresses);
}

void vds::_server_database::get_endpoints(std::map<std::string, std::string>& result)
{
  this->get_endpoints_query_.query(
    this->db_,
    "SELECT endpoint_id, addresses FROM endpoint",
    [&result](sql_statement & st)->bool{
      std::string endpoint_id;
      std::string addresses;
      st.get_value(0, endpoint_id);
      st.get_value(1, addresses);

      result.insert(std::pair<std::string, std::string>(endpoint_id, addresses));
      return true;
    });
}


/////////////////////////////////////////////

vds::principal_log_record
  vds::_server_database::add_local_record(
    const service_provider & sp,
    const principal_log_record::record_id & record_id,
    const guid & principal_id,
    const std::shared_ptr<json_value> & message,
    const vds::asymmetric_private_key & principal_private_key,
    const_data_buffer & signature)
{
  std::list<principal_log_record::record_id> parents;
  auto max_order_num = this->get_current_state(sp, parents);

  std::lock_guard<std::mutex> lock(this->principal_log_mutex_);

  //Sign message
  principal_log_record result(record_id, principal_id, parents, message, max_order_num + 1);
  std::string body = result.serialize(false)->str();
  signature = asymmetric_sign::signature(
    hash::sha256(),
    principal_private_key,
    body.c_str(),
    body.length());

  //Register message
  this->add_principal_log(
    sp,
    record_id,
    principal_id,
    message->str(),
    signature,
    max_order_num + 1,
    iserver_database::principal_log_state::front);

  //update tails & create links
  for (auto& p : parents) {
    this->principal_log_update_state(
      sp,
      p,
      iserver_database::principal_log_state::processed);

    this->principal_log_add_link(
      sp,
      p,
      record_id);
  }

  return result;
}

bool vds::_server_database::save_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  std::lock_guard<std::mutex> lock(this->principal_log_mutex_);
  
  auto state = this->principal_log_get_state(sp, record.id());
  if (state != iserver_database::principal_log_state::not_found) {
    return false;
  }

  state = iserver_database::principal_log_state::front;
  for (auto& p : record.parents()) {
    auto parent_state = this->principal_log_get_state(sp, p);
    if(iserver_database::principal_log_state::not_found == parent_state
      || iserver_database::principal_log_state::stored == parent_state
      || iserver_database::principal_log_state::front == parent_state) {
      state = iserver_database::principal_log_state::stored;
      break;
    }
  }

  this->add_principal_log(
    sp,
    record.id(),
    record.principal_id(),
    record.message()->str(),
    signature,
    record.order_num(),
    state);
  
  for (auto& p : record.parents()) {
    this->principal_log_add_link(
      sp,
      p,
      record.id());
  }

  return true;
}

void vds::_server_database::principal_log_add_link(
  const service_provider & sp,
  const guid & source_id,
  const guid & target_id)
{
  this->principal_log_add_link_statement_.execute(
    this->db_,
    "INSERT INTO principal_log_link (parent_id,follower_id)\
       VALUES (@parent_id,@follower_id)",
    source_id,
    target_id);
}

void vds::_server_database::add_principal_log(
  const service_provider & sp,
  const guid & record_id,
  const guid & principal_id,
  const std::string & body,
  const const_data_buffer & signature,
  int order_num,
  iserver_database::principal_log_state state)
{
  this->principal_log_add_statement_.execute(
    this->db_,
    "INSERT INTO principal_log (id,principal_id,message,signature,order_num,state)\
    VALUES (@id,@principal_id,@message,@signature,@order_num,@state)",
    record_id,
    principal_id,
    body,
    signature,
    order_num,
    (int)state);
}

void vds::_server_database::principal_log_update_state(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  iserver_database::principal_log_state state)
{
  this->principal_log_update_state_statement_.execute(
    this->db_,
    "UPDATE principal_log SET state=?2 WHERE id=?1",
    record_id,
    (int)state);
}

vds::iserver_database::principal_log_state
vds::_server_database::principal_log_get_state(
  const service_provider & sp,
  const principal_log_record::record_id & record_id)
{
  vds::iserver_database::principal_log_state result = iserver_database::principal_log_state::not_found;

  this->principal_log_get_state_query_.query(
    this->db_,
    "SELECT state FROM principal_log WHERE id=@id",
    [&result](sql_statement & st)->bool {

      int value;
      if (st.get_value(0, value)) {
        result = (iserver_database::principal_log_state)value;
      }

      return false;
    },
    record_id);

  return result;
}

size_t vds::_server_database::get_current_state(const service_provider & sp, std::list<guid>& active_records)
{
  std::lock_guard<std::mutex> lock(this->principal_log_mutex_);

  size_t max_order_num = 0;
  //Collect parents
  this->get_principal_log_tails_query_.query(
    this->db_,
    ("SELECT id,order_num FROM principal_log WHERE state="
      + std::to_string((int)iserver_database::principal_log_state::tail)).c_str(),
    [&active_records, &max_order_num](sql_statement & reader)->bool {

    guid id;
    size_t order_num;
    reader.get_value(0, id);
    reader.get_value(1, order_num);

    if (max_order_num < order_num) {
      max_order_num = order_num;
    }

    active_records.push_back(id);
    return true;
  });

  return max_order_num;
}

void vds::_server_database::principal_log_get_parents(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  std::list<principal_log_record::record_id>& parents)
{
  this->principal_log_get_parents_query_.query(
    this->db_,
    "SELECT parent_id \
    FROM principal_log_link \
    WHERE follower_id=@follower_id",
    [&parents](sql_statement & st) -> bool{
      guid parent_id;

      st.get_value(0, parent_id);

      parents.push_back(parent_id);
      return true;
    },
    record_id);
}

void vds::_server_database::principal_log_get_followers(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  std::list<principal_log_record::record_id>& followers)
{
  this->principal_log_get_followers_query_.query(
    this->db_,
    "SELECT follower_id \
    FROM principal_log_link \
    WHERE parent_id=@parent_id",
    [&followers](sql_statement & st) -> bool {
      guid follower_id;

      st.get_value(0, follower_id);

      followers.push_back(follower_id);
      return true;
    },
    record_id);
}

void vds::_server_database::processed_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  std::lock_guard<std::mutex> lock(this->principal_log_mutex_);

  std::list<principal_log_record::record_id> parents;
  this->principal_log_get_parents(sp, id, parents);
  for (auto& p : parents) {
    auto parent_state = this->principal_log_get_state(sp, p);
    if (iserver_database::principal_log_state::tail == parent_state) {
      this->principal_log_update_state(sp, p, iserver_database::principal_log_state::processed);
      break;
    }
    else if (iserver_database::principal_log_state::processed != parent_state) {
      throw std::runtime_error("Invalid state");
    }
  }

  std::list<principal_log_record::record_id> followers;
  this->principal_log_get_followers(sp, id, followers);

  if (0 == followers.size()) {
    this->principal_log_update_state(sp, id, iserver_database::principal_log_state::tail);
  }
  else {
    this->principal_log_update_state(sp, id, iserver_database::principal_log_state::processed);
    
    for (auto& f : followers) {
      auto state = this->principal_log_get_state(sp, f);
      switch (state) {
      case iserver_database::principal_log_state::stored:
      {
        std::list<principal_log_record::record_id> parents;
        this->principal_log_get_parents(sp, f, parents);

        auto new_state = iserver_database::principal_log_state::front;
        for (auto& p : parents) {
          auto state = this->principal_log_get_state(sp, p);
          if (iserver_database::principal_log_state::stored == state
            || iserver_database::principal_log_state::front == state) {
            new_state = iserver_database::principal_log_state::stored;
            break;
          }

          if (iserver_database::principal_log_state::processed != state) {
            throw std::runtime_error("Invalid state");
          }
        }

        if (state != new_state) {
          this->principal_log_update_state(sp, f, new_state);
        }

        break;
      }
      default:
        throw std::runtime_error("State error");
      }
    }
  }
}

uint64_t vds::_server_database::get_principal_log_max_index(
  const service_provider & sp,
  const guid & id)
{
  uint64_t result = 0;

  this->get_principal_log_max_index_query_.query(
    this->db_,
    "SELECT MAX(source_index) FROM principal_log WHERE source_id=@source_id",
    [&result](sql_statement & st)->bool {

      st.get_value(0, result);
      return false;
    },
    id);

  return result;
}

void vds::_server_database::get_unknown_records(
  const service_provider & sp,
  std::list<principal_log_record::record_id>& result)
{
  this->get_unknown_records_query_.query(
    this->db_,
    "SELECT parent_id \
     FROM principal_log_link \
     WHERE NOT EXISTS (\
      SELECT * \
      FROM principal_log \
      WHERE principal_log.id=principal_log_link.parent_id)",
    [&result](sql_statement & st)->bool {

    principal_log_record::record_id item;

    st.get_value(0, item);

    result.push_back(item);

    return false;
  });
}

bool vds::_server_database::get_record(
  const service_provider & sp,
  const principal_log_record::record_id & id,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  bool result = false;

  std::string message;
  guid principal_id;
  size_t order_num;

  this->principal_log_get_query_.query(
    this->db_,
    "SELECT principal_id, message, signature, order_num FROM principal_log WHERE id=@id",
    [&result, &message, &principal_id, &result_signature, &order_num](sql_statement & st)->bool {

      st.get_value(0, principal_id);
      st.get_value(1, message);
      st.get_value(2, result_signature);
      st.get_value(3, order_num);
      
      result = true;

      return false;
    },
    id);

  if (result) {
    std::list<principal_log_record::record_id> parents;
    this->principal_log_get_parents(sp, id, parents);

  std::shared_ptr<json_value> body;
    dataflow(
      dataflow_arguments<char>(message.c_str(), message.length()),
      json_parser("Message body"),
      dataflow_require_once<std::shared_ptr<json_value>>(&body)
    )(
      [&id, &result_record, &parents, &body, principal_id, order_num](const service_provider & sp) {
        result_record.reset(
          id,
          principal_id,
          parents,
          body,
          order_num);
      },
      [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        std::rethrow_exception(std::make_exception_ptr(*ex));
      },
      sp);
  }

  return result;
}

bool vds::_server_database::get_front_record(
  const service_provider & sp,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return this->get_record_by_state(
    sp,
    iserver_database::principal_log_state::front,
    result_record,
    result_signature);    
}

void vds::_server_database::delete_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  throw std::runtime_error("Not implemented");
}

bool vds::_server_database::get_record_by_state(
  const service_provider & sp,
  iserver_database::principal_log_state state,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  bool result = false;

  principal_log_record::record_id id;
  guid principal_id;
  std::string message;
  int order_num;

  this->get_record_by_state_query_.query(
    this->db_,
    "SELECT id,principal_id,message,order_num,signature FROM  principal_log WHERE state=@state LIMIT 1",
    [&result, &id, &principal_id, &message, &order_num, &result_signature](sql_statement & st)->bool {

      st.get_value(0, id);
      st.get_value(1, principal_id);
      st.get_value(2, message);
      st.get_value(3, order_num);
      st.get_value(4, result_signature);

      result = true;

      return false;
    },
    (int)state);

  if (result) {
    std::list<principal_log_record::record_id> parents;
    this->principal_log_get_parents(sp, id, parents);

    std::shared_ptr<json_value> body;
    dataflow(
      dataflow_arguments<char>(message.c_str(), message.length()),
      json_parser("Message body"),
      dataflow_require_once<std::shared_ptr<json_value>>(&body)
    )(
      [&id, principal_id, &result_record, &parents, &body, order_num](
        const service_provider & sp) {
        result_record.reset(
          id,
          principal_id,
          parents,
          body,
          order_num);
      },
      [](const service_provider & sp,
         const std::shared_ptr<std::exception> & ex) {
        std::rethrow_exception(std::make_exception_ptr(*ex)); 
      },
      sp);
  }

  return result;  
}

size_t vds::_server_database::get_last_chunk(
  const service_provider & sp,
  const guid & server_id)
{  
  size_t result = 0;
  auto st = this->db_.parse("SELECT MAX(chunk_index) FROM object_chunk WHERE server_id=@server_id");
  st.set_parameter(0, server_id);
  
  while(st.execute()) {
    st.get_value(0, result);
  }

  return result;
}

size_t vds::_server_database::get_tail_chunk(
  const service_provider & sp,
  const guid & server_id,
  size_t & result_size)
{
  size_t result = 0;
  auto st = this->db_.parse("SELECT chunk_index FROM tmp_object_chunk WHERE server_id=@server_id");
  st.set_parameter(0, server_id);
  
  while(st.execute()) {
    st.get_value(0, result);
  }
  
  st = this->db_.parse("SELECT SUM(length(data)) FROM tmp_object_chunk_map WHERE server_id=@server_id AND chunk_index=@chunk_index");
  st.set_parameter(0, server_id);
  st.set_parameter(1, result);
  
  while(st.execute()) {
    st.get_value(0, result_size);
  }

  return result;
}


void vds::_server_database::add_full_chunk(
  const service_provider & sp,
  const guid & object_id,
  size_t offset,
  size_t size,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index)
{
  this->add_chunk(sp, server_id, index, size, object_hash);
  this->add_object_chunk_map(sp, server_id, index, object_id, offset, 0, size, object_hash);
}

void vds::_server_database::add_chunk(
  const service_provider & sp,
  const guid & server_id,
  size_t index,
  size_t size,
  const const_data_buffer & object_hash)
{
  this->add_chunk_statement_.execute(
    this->db_,
    "INSERT INTO object_chunk(server_id, chunk_index, chunk_size, hash)\
    VALUES (@server_id, @chunk_index, @chunk_size, @hash)",
    server_id,
    index,
    size,
    object_hash);
}

void vds::_server_database::add_object_chunk_map(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index,
  const guid & object_id,
  size_t object_offset,
  size_t chunk_offset,
  size_t length,
  const const_data_buffer & hash)
{
  this->object_chunk_map_statement_.execute(
    this->db_,
    "INSERT INTO object_chunk_map(server_id,chunk_index,object_id,object_offset,chunk_offset,length,hash)\
    VALUES(@server_id,@chunk_index,@object_id,@object_offset,@chunk_offset,@length,@hash)",
    server_id,
    chunk_index,
    object_id,
    object_offset,
    chunk_offset,
    length,
    hash);
}

void vds::_server_database::start_tail_chunk(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index)
{
  this->add_tmp_chunk_statement_.execute(
    this->db_,
    "INSERT INTO tmp_object_chunk(server_id, chunk_index)\
    VALUES (@server_id, @chunk_index)",
    server_id,
    chunk_index);
}

void vds::_server_database::add_tail_object_chunk_map(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index,
  const guid & object_id,
  size_t object_offset,
  size_t chunk_offset,
  const const_data_buffer & hash,
  const const_data_buffer & data)
{
  this->tmp_object_chunk_map_statement_.execute(
    this->db_,
    "INSERT INTO tmp_object_chunk_map(server_id,chunk_index,object_id,object_offset,chunk_offset,hash,data)\
    VALUES(@server_id,@chunk_index,@object_id,@object_offset,@chunk_offset,@hash,@data)",
    server_id,
    chunk_index,
    object_id,
    object_offset,
    chunk_offset,
    hash,
    data);
}

void vds::_server_database::final_tail_chunk(
  const service_provider & sp,
  size_t chunk_length,
  const const_data_buffer & chunk_hash,
  const guid & server_id,
  size_t chunk_index)
{
  this->add_chunk(sp, server_id, chunk_index, chunk_length, chunk_hash);

  this->move_object_chunk_map_statement_.execute(
    this->db_,
    "INSERT INTO object_chunk_map(server_id,chunk_index,object_id,object_offset,chunk_offset,length,hash)\
    SELECT server_id,chunk_index,object_id,object_offset,chunk_offset,length,hash\
    FROM tmp_object_chunk_map WHERE server_id=@server_id AND chunk_index=@chunk_index",
    server_id,
    chunk_index);

  this->delete_tmp_object_chunk_statement_.execute(
    this->db_,
    "DELETE FROM tmp_object_chunk\
    WHERE server_id=@server_id AND chunk_index=@chunk_index",
    server_id,
    chunk_index);

  this->delete_tmp_object_chunk_map_statement_.execute(
    this->db_,
    "DELETE FROM tmp_object_chunk_map\
    WHERE server_id=@server_id AND chunk_index=@chunk_index",
    server_id,
    chunk_index);
}

void vds::_server_database::add_to_tail_chunk(
  const service_provider & sp,
  const guid & object_id,
  size_t offset,
  const const_data_buffer & object_hash,
  const guid & server_id,
  size_t index,
  size_t chunk_offset,
  const const_data_buffer & data)
{
  this->add_tail_object_chunk_map(sp, server_id, index, object_id, offset, chunk_offset, object_hash, data);
}

void vds::_server_database::add_chunk_replica(
  const service_provider & sp,
  const guid & server_id,
  size_t index,
  uint16_t replica,
  size_t replica_length,
  const const_data_buffer & replica_hash)
{
  this->add_chunk_replica_statement_.execute(
    this->db_,
    "INSERT INTO object_chunk_replica(server_id,chunk_index,replica,replica_length,replica_hash)\
    VALUES(@server_id,@chunk_index,@replica,@replica_length,@replica_hash)",
    server_id,
    index,
    replica,
    replica_length,
    replica_hash);
}

void vds::_server_database::add_chunk_store(
  const service_provider & sp,
  const guid & server_id,
  size_t index,
  uint16_t replica,
  const guid & storage_id,
  const const_data_buffer & data)
{
  this->add_object_chunk_store_statement_.execute(
    this->db_,
    "INSERT INTO object_chunk_store(server_id,chunk_index,replica,storage_id,data)\
    VALUES(@server_id,@chunk_index,@replica,@storage_id,@data)",
    server_id,
    index,
    replica,
    storage_id,
    data);
}

vds::const_data_buffer vds::_server_database::get_tail_data(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index)
{
  std::vector<uint8_t> data;

  this->get_tail_data_query_.query(
    this->db_,
    "SELECT data,chunk_offset\
     FROM tmp_object_chunk_map\
     WHERE server_id=@server_id AND chunk_index=@chunk_index\
     ORDER BY chunk_offset",
    [&data](sql_statement & st)->bool {
      vds::const_data_buffer item;
      size_t offset;

      st.get_value(0, item);
      st.get_value(1, offset);

      if (offset != data.size()) {
        throw std::runtime_error("Database is corrupted");
      }

      data.insert(data.end(), item.data(), item.data() + item.size());
      return true;
    },    
    server_id,
    chunk_index);

  return const_data_buffer(data);
}
