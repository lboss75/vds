/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_database.h"
#include "server_database_p.h"
#include "cert_record.h"
#include "storage_log.h"

void vds::iserver_database::add_cert(
  const service_provider & sp,
  const cert_record & record)
{
  static_cast<_server_database *>(this)->add_cert(sp, record);
}

std::unique_ptr<vds::cert_record> vds::iserver_database::find_cert(
  const service_provider & sp,
  const std::string & object_name)
{
  return static_cast<_server_database *>(this)->find_cert(
    sp,
    object_name);
}

void vds::iserver_database::add_object(
  const service_provider & sp,
  const guid & server_id,
  const server_log_new_object & index)
{
  static_cast<_server_database *>(this)->add_object(sp, server_id, index);
}

void vds::iserver_database::add_file(
  const service_provider & sp,
  const guid & server_id,
  const server_log_file_map & fm)
{
  static_cast<_server_database *>(this)->add_file(sp, server_id, fm);
}

void vds::iserver_database::get_file_versions(
  const service_provider & sp,
  const std::string & user_login,
  const std::string & name,
  std::list<server_log_file_version>& result)
{
  static_cast<_server_database *>(this)->get_file_versions(sp, user_login, name, result);
}

std::unique_ptr<vds::server_log_file_map> vds::iserver_database::get_file_version_map(
  const service_provider & sp, 
  const guid & server_id,
  const std::string & version_id)
{
  return static_cast<_server_database *>(this)->get_file_version_map(sp, server_id, version_id);
}

uint64_t vds::iserver_database::last_object_index(
  const service_provider & sp, 
  const guid& server_id)
{
  return static_cast<_server_database *>(this)->last_object_index(sp, server_id);
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
uint64_t vds::iserver_database::get_server_log_max_index(
  const service_provider & sp,
  const guid & id)
{
  return static_cast<_server_database *>(this)->get_server_log_max_index(sp, id);
}

vds::server_log_record vds::iserver_database::add_local_record(
  const service_provider & sp,
  const server_log_record::record_id & record_id,
  const std::shared_ptr<json_value> & message,
  const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->add_local_record(sp, record_id, message, signature);
}

bool vds::iserver_database::save_record(
  const service_provider & sp,
  const server_log_record & record,
  const const_data_buffer & signature)
{
  return static_cast<_server_database *>(this)->save_record(sp, record, signature);
}

void vds::iserver_database::get_unknown_records(
  const service_provider & sp,
  std::list<server_log_record::record_id>& result)
{
  static_cast<_server_database *>(this)->get_unknown_records(sp, result);
}

bool vds::iserver_database::get_record(
  const service_provider & sp,
  const server_log_record::record_id & id,
  server_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_record(sp, id, result_record, result_signature);
}

bool vds::iserver_database::get_front_record(
  const service_provider & sp,
  server_log_record & result_record,
  const_data_buffer & result_signature)
{
  return static_cast<_server_database *>(this)->get_front_record(sp, result_record, result_signature);
}

void vds::iserver_database::processed_record(
  const service_provider & sp,
  const server_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->processed_record(sp, id);
}

void vds::iserver_database::delete_record(
  const service_provider & sp,
  const server_log_record::record_id & id)
{
  static_cast<_server_database *>(this)->delete_record(sp, id);
}

vds::iserver_database::server_log_state vds::iserver_database::get_record_state(
  const service_provider & sp,
  const server_log_record::record_id & id)
{
  return static_cast<_server_database *>(this)->server_log_get_state(sp, id);
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
      "CREATE TABLE object(\
      server_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      original_lenght INTEGER NOT NULL,\
      original_hash VARCHAR(64) NOT NULL,\
      target_lenght INTEGER NOT NULL, \
      target_hash VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_objects PRIMARY KEY (server_id, object_index))");

    this->db_.execute(
      "CREATE TABLE cert(\
      object_name VARCHAR(64) PRIMARY KEY NOT NULL,\
      body TEXT NOT NULL,\
      key TEXT NOT NULL,\
      password_hash VARCHAR(64) NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE endpoint(\
      endpoint_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      addresses TEXT NOT NULL)");

    this->db_.execute(
      "CREATE TABLE file(\
      version_id VARCHAR(64) PRIMARY KEY NOT NULL,\
      server_id VARCHAR(64) NOT NULL,\
      user_login VARCHAR(64) NOT NULL,\
      name TEXT NOT NULL)");
    
    this->db_.execute(
      "CREATE TABLE file_map(\
      version_id VARCHAR(64) NOT NULL,\
      object_index INTEGER NOT NULL,\
      order_num INTEGER NOT NULL,\
      CONSTRAINT pk_file_map PRIMARY KEY (version_id, object_index))");

    this->db_.execute(
      "CREATE TABLE server_log(\
      source_id VARCHAR(64) NOT NULL,\
      source_index INTEGER NOT NULL,\
      message TEXT NOT NULL,\
      signature VARCHAR(64) NOT NULL,\
      state INTEGER NOT NULL,\
      CONSTRAINT pk_server_log PRIMARY KEY(source_id, source_index))");

    this->db_.execute(
      "CREATE TABLE server_log_link(\
      parent_id VARCHAR(64) NOT NULL,\
      parent_index INTEGER NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
      follower_index INTEGER NOT NULL,\
      CONSTRAINT pk_server_log_link PRIMARY KEY(parent_id,parent_index,follower_id,follower_index))");


    this->db_.execute("INSERT INTO module(id, version, installed) VALUES('kernel', 1, datetime('now'))");
    this->db_.execute("INSERT INTO endpoint(endpoint_id, addresses) VALUES('default', 'udp://127.0.0.1:8050;https://127.0.0.1:8050')");
  }
}

void vds::_server_database::stop(const service_provider & sp)
{
  this->db_.close();
}

void vds::_server_database::add_cert(
  const service_provider & sp,
  const cert_record & record)
{
  this->add_cert_statement_.execute(
    this->db_,
    "INSERT INTO cert(object_name, body, key, password_hash)\
      VALUES (@object_name, @body, @key, @password_hash)",

    record.object_name(),
    record.cert_body(),
    record.cert_key(),
    record.password_hash());
}

std::unique_ptr<vds::cert_record> vds::_server_database::find_cert(
  const service_provider & sp,
  const std::string & object_name)
{
  std::unique_ptr<cert_record> result;
  this->find_cert_query_.query(
    this->db_,
    "SELECT body, key, password_hash\
     FROM cert\
     WHERE object_name=@object_name",
    [&result, object_name](sql_statement & st)->bool{
      
      std::string body;
      std::string key;
      const_data_buffer password_hash;
      
      st.get_value(0, body);
      st.get_value(1, key);
      st.get_value(2, password_hash);

      result.reset(new cert_record(
        object_name,
        body,
        key,
        password_hash));
      
      return false; },
    object_name);
  
  return result;
}

void vds::_server_database::add_object(
  const service_provider & sp,
  const guid & server_id,
  const server_log_new_object & index)
{
  this->add_object_statement_.execute(
    this->db_,
    "INSERT INTO object(server_id, object_index, original_lenght, original_hash, target_lenght, target_hash)\
    VALUES (@server_id, @object_index, @original_lenght, @original_hash, @target_lenght, @target_hash)",
    server_id,
    index.index(),
    index.original_lenght(),
    index.original_hash(),
    index.target_lenght(),
    index.target_hash());
}

uint64_t vds::_server_database::last_object_index(
  const service_provider & sp,
  const guid& server_id)
{
  uint64_t result = 0;
  this->last_object_index_query_.query(
    this->db_,
    "SELECT MAX(object_index)+1 FROM object WHERE server_id=@server_id",
    [&result](sql_statement & st)->bool{
      st.get_value(0, result);
      return false;
    },
    server_id);
  
  return result;
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

void vds::_server_database::add_file(
  const service_provider & sp,
  const guid & server_id,
  const server_log_file_map & fm)
{
    this->add_file_statement_.execute(
      this->db_,
      "INSERT INTO file(version_id,server_id,user_login,name)\
      VALUES(@version_id,@server_id,@user_login,@name)",
      fm.version_id(),
      server_id,
      fm.user_login(),
      fm.name());
    
    int index = 0;
    for(auto & item : fm.items()){
      this->add_file_map_statement_.execute(
        this->db_,
        "INSERT INTO file_map(version_id,object_index,order_num)\
        VALUES(@version_id,@object_index,@order_num)",
        fm.version_id(),
        item.index(),
        index++);
    }
}

void vds::_server_database::get_file_versions(
  const service_provider & sp,
  const std::string & user_login,
  const std::string & name,
  std::list<server_log_file_version> & result)
{
  this->get_file_versions_query_.query(
    this->db_,
    "SELECT version_id,server_id FROM file\
     WHERE user_login=@user_login AND name=@name",
    [&result](sql_statement & reader)->bool {

     std::string version_id;
     guid server_id;
    reader.get_value(0, version_id);
    reader.get_value(1, server_id);

    result.push_back(server_log_file_version(version_id, server_id));
    return true;
  },
    user_login,
    name);
}

std::unique_ptr<vds::server_log_file_map> vds::_server_database::get_file_version_map(
  const service_provider & sp,
  const guid & server_id,
  const std::string & version_id)
{
  std::unique_ptr<server_log_file_map> result;

  this->get_file_version_info_query_.query(
    this->db_,
    "SELECT user_login,server_id,name,meta_info FROM file\
     WHERE version_id=@version_id",
    [&result, version_id](sql_statement & reader)->bool {
    std::string user_login;
    guid server_id;
    std::string name;
    const_data_buffer meta_info;

    reader.get_value(0, user_login);
    reader.get_value(1, server_id);
    reader.get_value(2, name);
    reader.get_value(3, meta_info);

    result.reset(new server_log_file_map(version_id, user_login, name, meta_info));
    return false;
    },
    version_id);

  if (!result) {
    return result;
  }

  this->get_file_version_map_query_.query(
    this->db_,
    "SELECT fm.object_index,ob.original_lenght,ob.original_hash,ob.target_lenght,ob.target_hash FROM file_map fm\
     INNER JOIN object ob\
     ON ob.object_index=fm.object_index\
     WHERE ob.server_id=@server_id AND fm.version_id=@version_id\
     ORDER BY fm.order_num",
    [&result](sql_statement & reader)->bool {

    uint64_t index;
    uint64_t original_lenght;
    const_data_buffer original_hash;
    uint64_t target_lenght;
    const_data_buffer target_hash;

    reader.get_value(0, index);
    reader.get_value(1, original_lenght);
    reader.get_value(2, original_hash);
    reader.get_value(3, target_lenght);
    reader.get_value(4, target_hash);

    result->add(server_log_new_object(index, original_lenght, original_hash, target_lenght, target_hash));
    return true;
  },
    version_id);

  return result;
}

/////////////////////////////////////////////

vds::server_log_record
  vds::_server_database::add_local_record(
    const service_provider & sp,
    const server_log_record::record_id & record_id,
    const std::shared_ptr<json_value> & message,
    const_data_buffer & signature)
{
  std::lock_guard<std::mutex> lock(this->server_log_mutex_);

  std::list<server_log_record::record_id> parents;

  //Collect parents
  this->get_server_log_tails_query_.query(
    this->db_,
    ("SELECT source_id,source_index FROM server_log WHERE state="
      + std::to_string((int)iserver_database::server_log_state::tail)).c_str(),
    [&parents](sql_statement & reader)->bool {

      guid source_id;
      uint64_t source_index;
      reader.get_value(0, source_id);
      reader.get_value(1, source_index);

      parents.push_back(server_log_record::record_id{ source_id, source_index });
      return true;
  });

  //Sign message
  server_log_record result(record_id, parents, message);
  std::string body = server_log_record(record_id, parents, message).serialize(false)->str();
  signature = asymmetric_sign::signature(
    hash::sha256(),
    sp.get<istorage_log>()->server_private_key(),
    body.c_str(),
    body.length());

  //Register message
  this->add_server_log(
    sp,
    record_id.source_id,
    record_id.index,
    message->str(),
    signature,
    iserver_database::server_log_state::front);

  //update tails & create links
  for (auto& p : parents) {
    this->server_log_update_state(
      sp,
      p,
      iserver_database::server_log_state::processed);

    this->server_log_add_link(
      sp,
      p.source_id,
      p.index,
      record_id.source_id,
      record_id.index);
  }

  return result;
}

bool vds::_server_database::save_record(
  const service_provider & sp,
  const server_log_record & record,
  const const_data_buffer & signature)
{
  std::lock_guard<std::mutex> lock(this->server_log_mutex_);
  
  auto state = this->server_log_get_state(sp, record.id());
  if (state != iserver_database::server_log_state::not_found) {
    return false;
  }

  state = iserver_database::server_log_state::front;
  for (auto& p : record.parents()) {
    auto parent_state = this->server_log_get_state(sp, p);
    if(iserver_database::server_log_state::not_found == parent_state
      || iserver_database::server_log_state::stored == parent_state
      || iserver_database::server_log_state::front == parent_state) {
      state = iserver_database::server_log_state::stored;
      break;
    }
  }

  this->add_server_log(
    sp,
    record.id().source_id,
    record.id().index,
    record.message()->str(),
    signature,
    state);
  
  for (auto& p : record.parents()) {
    this->server_log_add_link(
      sp,
      p.source_id,
      p.index,
      record.id().source_id,
      record.id().index);
  }

  return true;
}

void vds::_server_database::server_log_add_link(
  const service_provider & sp,
  const guid & source_id,
  uint64_t source_index,
  const guid & target_id,
  uint64_t target_index)
{
  this->server_log_add_link_statement_.execute(
    this->db_,
    "INSERT INTO server_log_link (parent_id,parent_index,follower_id,follower_index)\
       VALUES (@parent_id,@parent_index,@follower_id,@follower_index)",
    source_id,
    source_index,
    target_id,
    target_index);
}

void vds::_server_database::add_server_log(
  const service_provider & sp,
  const guid & source_id,
  uint64_t source_index,
  const std::string & body,
  const const_data_buffer & signature,
  iserver_database::server_log_state state)
{
  this->server_log_add_statement_.execute(
    this->db_,
    "INSERT INTO server_log (source_id,source_index,message,signature,state)\
    VALUES (@source_id,@source_index,@message,@signature,@state)",
    source_id,
    source_index,
    body,
    signature,
    (int)state);
}

void vds::_server_database::server_log_update_state(
  const service_provider & sp,
  const server_log_record::record_id & record_id,
  iserver_database::server_log_state state)
{
  this->server_log_update_state_statement_.execute(
    this->db_,
    "UPDATE server_log SET state=?3 WHERE source_id=?1 AND source_index=?2",
    record_id.source_id,
    record_id.index,
    (int)state);
}

vds::iserver_database::server_log_state
vds::_server_database::server_log_get_state(
  const service_provider & sp,
  const server_log_record::record_id & record_id)
{
  vds::iserver_database::server_log_state result = iserver_database::server_log_state::not_found;

  this->server_log_get_state_query_.query(
    this->db_,
    "SELECT state FROM server_log WHERE source_id=@source_id AND source_index=@source_index",
    [&result](sql_statement & st)->bool {

      int value;
      if (st.get_value(0, value)) {
        result = (iserver_database::server_log_state)value;
      }

      return false;
    },
    record_id.source_id,
    record_id.index);

  return result;
}

void vds::_server_database::server_log_get_parents(
  const service_provider & sp,
  const server_log_record::record_id & record_id,
  std::list<server_log_record::record_id>& parents)
{
  this->server_log_get_parents_query_.query(
    this->db_,
    "SELECT parent_id, parent_index \
    FROM server_log_link \
    WHERE follower_id=@follower_id AND follower_index=@follower_index",
    [&parents](sql_statement & st) -> bool{
      guid parent_id;
      uint64_t parent_index;

      st.get_value(0, parent_id);
      st.get_value(1, parent_index);

      parents.push_back(server_log_record::record_id{ parent_id, parent_index });
      return true;
    },
    record_id.source_id,
    record_id.index);
}

void vds::_server_database::server_log_get_followers(
  const service_provider & sp,
  const server_log_record::record_id & record_id,
  std::list<server_log_record::record_id>& followers)
{
  this->server_log_get_followers_query_.query(
    this->db_,
    "SELECT follower_id, follower_index \
    FROM server_log_link \
    WHERE parent_id=@parent_id AND parent_index=@parent_index",
    [&followers](sql_statement & st) -> bool {
      guid follower_id;
      uint64_t follower_index;

      st.get_value(0, follower_id);
      st.get_value(1, follower_index);

      followers.push_back(server_log_record::record_id{ follower_id, follower_index });
      return true;
    },
    record_id.source_id,
    record_id.index);
}

void vds::_server_database::processed_record(
  const service_provider & sp,
  const server_log_record::record_id & id)
{
  std::lock_guard<std::mutex> lock(this->server_log_mutex_);

  std::list<server_log_record::record_id> parents;
  this->server_log_get_parents(sp, id, parents);
  for (auto& p : parents) {
    auto parent_state = this->server_log_get_state(sp, p);
    if (iserver_database::server_log_state::tail == parent_state) {
      this->server_log_update_state(sp, p, iserver_database::server_log_state::processed);
      break;
    }
    else if (iserver_database::server_log_state::processed != parent_state) {
      throw std::runtime_error("Invalid state");
    }
  }

  std::list<server_log_record::record_id> followers;
  this->server_log_get_followers(sp, id, followers);

  if (0 == followers.size()) {
    this->server_log_update_state(sp, id, iserver_database::server_log_state::tail);
  }
  else {
    this->server_log_update_state(sp, id, iserver_database::server_log_state::processed);
    
    for (auto& f : followers) {
      auto state = this->server_log_get_state(sp, f);
      switch (state) {
      case iserver_database::server_log_state::stored:
      {
        std::list<server_log_record::record_id> parents;
        this->server_log_get_parents(sp, f, parents);

        auto new_state = iserver_database::server_log_state::front;
        for (auto& p : parents) {
          auto state = this->server_log_get_state(sp, p);
          if (iserver_database::server_log_state::stored == state
            || iserver_database::server_log_state::front == state) {
            new_state = iserver_database::server_log_state::stored;
            break;
          }

          if (iserver_database::server_log_state::processed != state) {
            throw std::runtime_error("Invalid state");
          }
        }

        if (state != new_state) {
          this->server_log_update_state(sp, f, new_state);
        }

        break;
      }
      default:
        throw std::runtime_error("State error");
      }
    }
  }
}

uint64_t vds::_server_database::get_server_log_max_index(
  const service_provider & sp,
  const guid & id)
{
  uint64_t result = 0;

  this->get_server_log_max_index_query_.query(
    this->db_,
    "SELECT MAX(source_index) FROM server_log WHERE source_id=@source_id",
    [&result](sql_statement & st)->bool {

      st.get_value(0, result);
      return false;
    },
    id);

  return result;
}

void vds::_server_database::get_unknown_records(
  const service_provider & sp,
  std::list<server_log_record::record_id>& result)
{
  this->get_unknown_records_query_.query(
    this->db_,
    "SELECT parent_id,parent_index \
     FROM server_log_link \
     WHERE NOT EXISTS (\
      SELECT * \
      FROM server_log \
      WHERE server_log.source_id=server_log_link.parent_id\
       AND server_log.source_index=server_log_link.parent_index)",
    [&result](sql_statement & st)->bool {

    server_log_record::record_id item;

    st.get_value(0, item.source_id);
    st.get_value(1, item.index);

    result.push_back(item);

    return false;
  });
}

bool vds::_server_database::get_record(
  const service_provider & sp,
  const server_log_record::record_id & id,
  server_log_record & result_record,
  const_data_buffer & result_signature)
{
  bool result = false;

  std::string message;

  this->server_log_get_query_.query(
    this->db_,
    "SELECT message, signature FROM  server_log WHERE source_id=@source_id AND source_index=@source_index",
    [&result, &message, &result_signature](sql_statement & st)->bool {

      st.get_value(0, message);
      st.get_value(1, result_signature);

      result = true;

      return false;
    },
    id.source_id,
    id.index);

  if (result) {
    std::list<server_log_record::record_id> parents;
    this->server_log_get_parents(sp, id, parents);

  std::shared_ptr<json_value> body;
    dataflow(
      dataflow_arguments<char>(message.c_str(), message.length()),
      json_parser("Message body"),
      dataflow_require_once<std::shared_ptr<json_value>>(&body)
    )(
      [&id, &result_record, &parents, &body](const service_provider & sp) {
        result_record.reset(
          id,
          parents,
          body);
      },
      [](const service_provider & sp, std::exception_ptr ex) { std::rethrow_exception(ex); },
      sp);
  }

  return result;
}

bool vds::_server_database::get_front_record(
  const service_provider & sp,
  server_log_record & result_record,
  const_data_buffer & result_signature)
{
  return this->get_record_by_state(
    sp,
    iserver_database::server_log_state::front,
    result_record,
    result_signature);    
}

void vds::_server_database::delete_record(
  const service_provider & sp,
  const server_log_record::record_id & id)
{
  throw std::runtime_error("Not implemented");
}

bool vds::_server_database::get_record_by_state(
  const service_provider & sp,
  iserver_database::server_log_state state,
  server_log_record & result_record,
  const_data_buffer & result_signature)
{
  bool result = false;

  server_log_record::record_id id;
  std::string message;

  this->get_record_by_state_query_.query(
    this->db_,
    "SELECT source_id,source_index,message,signature FROM  server_log WHERE state=@state LIMIT 1",
    [&result, &id, &message, &result_signature](sql_statement & st)->bool {

      st.get_value(0, id.source_id);
      st.get_value(1, id.index);
      st.get_value(2, message);
      st.get_value(3, result_signature);

      result = true;

      return false;
    },
    (int)state);

  if (result) {
    std::list<server_log_record::record_id> parents;
    this->server_log_get_parents(sp, id, parents);

    std::shared_ptr<json_value> body;
    dataflow(
      dataflow_arguments<char>(message.c_str(), message.length()),
      json_parser("Message body"),
      dataflow_require_once<std::shared_ptr<json_value>>(&body)
    )(
      [&id, &result_record, &parents, &body](
        const service_provider & sp) {
        result_record.reset(
          id,
          parents,
          body);
      },
      [](const service_provider & sp,
         std::exception_ptr ex) {
        std::rethrow_exception(ex); 
      },
      sp);
  }

  return result;  
}
