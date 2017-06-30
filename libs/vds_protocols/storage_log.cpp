/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "storage_log.h"
#include "storage_log_p.h"
#include "process_log_line.h"
#include "log_records.h"
#include "principal_record.h"
#include "node.h"
#include "endpoint.h"
#include "certificate_authority.h"
#include "certificate_authority_p.h"
#include "chunk_storage.h"
#include "server_log_sync.h"
#include "server_log_sync_p.h"
#include "server_certificate.h"

const vds::guid & vds::istorage_log::current_server_id() const
{
  return static_cast<const _storage_log *>(this)->current_server_id();
}

const vds::certificate & vds::istorage_log::server_certificate() const
{
  return static_cast<const _storage_log *>(this)->server_certificate();
}

const vds::asymmetric_private_key & vds::istorage_log::server_private_key() const
{
  return static_cast<const _storage_log *>(this)->server_private_key();
}

size_t vds::istorage_log::minimal_consensus() const
{
  return static_cast<const _storage_log *>(this)->minimal_consensus();
}

void vds::istorage_log::add_to_local_log(
  const service_provider & sp,
  const guid & principal_id,
  const vds::asymmetric_private_key & principal_private_key,
  const std::shared_ptr<json_value> & record,
  bool apply_record /*= true*/)
{
  static_cast<_storage_log *>(this)->add_to_local_log(sp, principal_id, principal_private_key, record, apply_record);
}

vds::async_task<> vds::istorage_log::register_server(
  const service_provider & sp,
  const guid & id,
  const guid & parent_id,
  const std::string & server_certificate,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
{
  return static_cast<_storage_log *>(this)->register_server(
    sp,
    id,
    parent_id,
    server_certificate,
    server_private_key,
    password_hash);
}

std::unique_ptr<vds::const_data_buffer> vds::istorage_log::get_object(
  const service_provider & sp,
  const vds::full_storage_object_id& object_id)
{
  return static_cast<_storage_log *>(this)->get_object(sp, object_id);
}

void vds::istorage_log::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  static_cast<_storage_log *>(this)->add_endpoint(sp, endpoint_id, addresses);
}

void vds::istorage_log::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  static_cast<_storage_log *>(this)->get_endpoints(sp, addresses);
}

void vds::istorage_log::reset(
  const service_provider & sp,
  const guid & principal_id,
  const vds::certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & root_password,
  const std::string & addresses)
{
  static_cast<_storage_log *>(this)->reset(
    sp,
    principal_id,
    root_certificate,
    private_key,
    root_password,
    addresses);
}

void vds::istorage_log::apply_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  static_cast<_storage_log *>(this)->apply_record(
    sp,
    record,
    signature);
}

vds::principal_log_record::record_id vds::istorage_log::get_last_applied_record(const service_provider & sp)
{
  return static_cast<_storage_log *>(this)->get_last_applied_record(sp);
}

///////////////////////////////////////////////////////////////////////////////
vds::_storage_log::_storage_log()
{
}

vds::_storage_log::~_storage_log()
{
}

void vds::_storage_log::reset(
  const service_provider & sp,
  const guid & principal_id,
  const certificate & root_certificate,
  const asymmetric_private_key & private_key,
  const std::string & password,
  const std::string & addresses
)
{
  this->vds_folder_ = foldername(persistence::current_user(sp), ".vds");
  this->vds_folder_.create();
  
  hash ph(hash::sha256());
  ph.update(password.c_str(), password.length());
  ph.final();
  
  this->add_to_local_log(
    sp,
    principal_id,
    private_key,
    server_log_root_certificate(
      principal_id,
      root_certificate.str(),
      private_key.str(password),
      ph.signature()).serialize(),
      true);
  this->add_to_local_log(sp, principal_id, private_key, server_log_new_server(
    this->current_server_id_,
    principal_id,
    this->server_certificate_.str(),
    this->current_server_key_.str(password),
    ph.signature()).serialize(),
    true);
  this->add_to_local_log(
    sp,
    principal_id,
    private_key,
    server_log_new_endpoint(this->current_server_id_, addresses).serialize(),
    true);
}


void vds::_storage_log::start(
  const service_provider & sp,
  const guid & current_server_id,
  const certificate & server_certificate,
  const asymmetric_private_key & server_private_key)
{
  this->server_certificate_ = server_certificate;
  this->current_server_key_ = server_private_key;
  this->current_server_id_ = current_server_id;
  this->vds_folder_ = foldername(persistence::current_user(sp), ".vds");
  this->process_timer_.start(sp, std::chrono::seconds(5), [this, sp](){
    return this->process_timer_jobs(sp);
  });
}

void vds::_storage_log::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);
}

vds::async_task<> vds::_storage_log::register_server(
  const service_provider & sp,
  const guid & id,
  const guid & parent_id,
  const std::string & server_certificate,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
{
  return create_async_task(
    [this, id, parent_id, server_certificate, server_private_key, password_hash](
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
    this->add_to_local_log(
      sp,
      this->current_server_id(),
      this->server_private_key(),
      server_log_new_server(
        id,
        parent_id,
        server_certificate,
        server_private_key,
        password_hash).serialize(),
      true);
    done(sp);
  });
}

void vds::_storage_log::add_to_local_log(
  const service_provider & sp,
  const guid & principal_id,
  const vds::asymmetric_private_key & principal_private_key,
  const std::shared_ptr<json_value> & message,
  bool apply_record)
{
  std::lock_guard<std::mutex> lock(this->record_state_mutex_);

  const_data_buffer signature;
  auto result = sp.get<iserver_database>()->add_local_record(
      sp,
      guid::new_guid(),
      principal_id,
      message,
      principal_private_key,
      signature);
    
  if(apply_record){
    this->apply_record(sp, result, signature);
  }
  else {
    this->last_applied_record_ = result.id();
    sp.get<iserver_database>()->processed_record(sp, result.id());
  }

  auto sl = sp.get<_server_log_sync>(false);
  if (nullptr != sl) {
    sl->on_new_local_record(sp, result, signature);
  }
}

void vds::_storage_log::apply_record(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  sp.get<logger>()->debug(sp, "Apply record %s", record.id().str().c_str());
  auto state = sp.get<iserver_database>()->get_record_state(sp, record.id());
  if(iserver_database::principal_log_state::front != state){
    throw std::runtime_error("Invalid server state");
  }
 
  auto obj = std::dynamic_pointer_cast<json_object>(record.message());
  if(!obj){
    sp.get<logger>()->info(sp, "Wrong messsage in the record %s", record.id().str().c_str());
    sp.get<iserver_database>()->delete_record(sp, record.id());
    return;
  }
  
  try {
    this->validate_signature(sp, record, signature);

    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      sp.get<logger>()->info(sp, "Missing messsage type in the record %s", record.id().str().c_str());
      sp.get<iserver_database>()->delete_record(sp, record.id());
      return;
    }
    
    if (principal_log_new_object::message_type == message_type) {
      principal_log_new_object msg(obj);
      sp.get<iserver_database>()->add_object(sp, msg);
    }
    else if (principal_log_new_object_map::message_type == message_type) {
      principal_log_new_object_map msg(obj);
      
      size_t offset = 0;
      for(auto & chunk : msg.full_chunks()){
        sp.get<iserver_database>()->add_full_chunk(
          sp,
          msg.object_id(),
          offset,
          msg.chunk_size(),
          chunk.chunk_hash(),
          msg.server_id(),
          chunk.chunk_index());
        
        size_t replica = 0;
        for(auto & replica_hash : chunk.replica_hashes()){
          sp.get<iserver_database>()->add_chunk_replica(
            sp,
            msg.server_id(),
            chunk.chunk_index(),
            replica++,
            msg.replica_length(),
            replica_hash);
        }
        
        offset += msg.chunk_size();
      }
      
      for(auto & tail_chunk : msg.tail_chunk_items()){
        sp.get<iserver_database>()->add_object_chunk_map(
          sp,
          tail_chunk.server_id(),
          tail_chunk.chunk_index(),
          tail_chunk.object_id(),
          tail_chunk.object_offset(),
          tail_chunk.chunk_offset(),
          tail_chunk.length(),
          tail_chunk.hash());
      }
    }
    else if(server_log_root_certificate::message_type == message_type){
      server_log_root_certificate msg(obj);
      
      sp.get<iserver_database>()->add_user_principal(
        sp,
        "root",
        vds::principal_record(
          msg.id(),
          msg.id(),
          msg.user_cert(),
          msg.user_private_key(),
          msg.password_hash()));
    }
    else if(server_log_new_server::message_type == message_type){
      server_log_new_server msg(obj);

      sp.get<iserver_database>()->add_principal(
        sp,
        vds::principal_record(
          msg.parent_id(),
          msg.id(),
          msg.server_cert(),
          msg.server_private_key(),
          msg.password_hash()));

    }
    else if(server_log_new_endpoint::message_type == message_type){
      server_log_new_endpoint msg(obj);
      
      sp.get<iserver_database>()->add_endpoint(
        sp,
        msg.server_id().str(),
        msg.addresses());
    }
    else {
      sp.get<logger>()->info(sp, "Unexpected messsage type '%s' in the record %s",
        message_type.c_str(),
        record.id().str().c_str());

      sp.get<iserver_database>()->delete_record(sp, record.id());
      return;
    }

    this->last_applied_record_ = record.id();
  }
  catch (const std::exception & ex) {
    sp.get<logger>()->info(sp, "%s", ex.what());
    sp.get<iserver_database>()->delete_record(sp, record.id());
    return;
  }
  catch(...){
    sp.get<logger>()->info(sp, "Unhandled error");
    sp.get<iserver_database>()->delete_record(sp, record.id());
    return;
  }
  
  sp.get<iserver_database>()->processed_record(sp, record.id());
}

std::unique_ptr<vds::const_data_buffer> vds::_storage_log::get_object(
  const service_provider & sp,
  const vds::full_storage_object_id& object_id)
{
  return sp.get<ilocal_cache>()->get_object(sp, object_id);
}

void vds::_storage_log::add_endpoint(
  const service_provider & sp,
  const std::string & endpoint_id,
  const std::string & addresses)
{
  sp.get<iserver_database>()->add_endpoint(sp, endpoint_id, addresses);
}

void vds::_storage_log::get_endpoints(
  const service_provider & sp,
  std::map<std::string, std::string> & addresses)
{
  sp.get<iserver_database>()->get_endpoints(sp, addresses);
}


  /*
  auto signature = asymmetric_sign::signature(hash::sha256(), this->current_server_key_, s.data());
  this->db_.add_object(this->current_server_id_, index, signature);
  return vds::storage_object_id(index, signature);
  */


bool vds::_storage_log::process_timer_jobs(const service_provider & sp)
{
  std::lock_guard<std::mutex> lock(this->record_state_mutex_);

  principal_log_record record;
  const_data_buffer signature;
  while(sp.get<iserver_database>()->get_front_record(sp, record, signature)){
    this->apply_record(sp, record, signature);
  }

  return true;
}

vds::asymmetric_public_key vds::_storage_log::corresponding_public_key(
  const service_provider & sp,
  const principal_log_record & record)
{
  auto principal = sp.get<iserver_database>()->find_principal(sp, record.principal_id());
  if (!principal) {
    auto obj = std::dynamic_pointer_cast<json_object>(record.message());
    if (!obj) {
      throw std::runtime_error("Wrong messsage in the record");
    }

    std::string message_type;
    if (!obj->get_property("$t", message_type)) {
      throw std::runtime_error("Missing messsage type in the record");
    }

    if (server_log_root_certificate::message_type == message_type) {
      server_log_root_certificate msg(obj);
      return certificate::parse(msg.user_cert()).public_key();
    }

    throw std::runtime_error("Principial not found");
  }
  else {
    return certificate::parse(principal->cert_body()).public_key();
  }
}

void vds::_storage_log::validate_signature(
  const service_provider & sp,
  const principal_log_record & record,
  const const_data_buffer & signature)
{

  std::string body = record.serialize(false)->str();
  if (!asymmetric_sign_verify::verify(
    hash::sha256(),
    this->corresponding_public_key(sp, record),
    signature,
    body.c_str(),
    body.length())) {
    sp.get<logger>()->trace(sp, "Wrong signature [%s] signed [%s]", body.c_str(), base64::from_bytes(signature).c_str());

    throw std::runtime_error("Invalid signature");
  }
}

void vds::_storage_log::create_database_objects(
  const service_provider & sp,
  uint64_t db_version,
  database_transaction & t)
{
  if (1 > db_version) {
    t.execute(
      "CREATE TABLE principal(\
      id VARCHAR(64) NOT NULL,\
      cert TEXT NOT NULL,\
      key TEXT NOT NULL,\
      password_hash BLOB NOT NULL,\
      parent VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_principal PRIMARY KEY(id))");

    t.execute(
      "CREATE TABLE user_principal(\
      id VARCHAR(64) NOT NULL,\
      login VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_user_principal PRIMARY KEY(id))");
    
    t.execute(
      "CREATE TABLE principal_log(\
      id VARCHAR(64) NOT NULL,\
      principal_id VARCHAR(64) NOT NULL,\
      message TEXT NOT NULL,\
      signature BLOB NOT NULL,\
      order_num INTEGER NOT NULL,\
      state INTEGER NOT NULL,\
      CONSTRAINT pk_principal_log PRIMARY KEY(id))");

    t.execute(
      "CREATE TABLE principal_log_link(\
      parent_id VARCHAR(64) NOT NULL,\
      follower_id VARCHAR(64) NOT NULL,\
      CONSTRAINT pk_principal_log_link PRIMARY KEY(parent_id,follower_id))");
  }
}

void vds::_storage_log::add_principal(
  const service_provider & sp,
  const principal_record & record)
{
  principal t;
  
  database_transaction::current(sp)
  .insert_into(t)
  .set(
    t.id = record.id(),
    t.cert = record.cert_body(),
    t.key = record.cert_key(),
    t.password_hash = record.password_hash(),
    t.parent = record.parent_principal())
  .execute();
}

void vds::_storage_log::add_user_principal(
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

vds::guid vds::_storage_log::get_root_principal(
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

std::unique_ptr<vds::principal_record> vds::_storage_log::find_principal(
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

std::unique_ptr<vds::principal_record> vds::_storage_log::find_user_principal(
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

void vds::_storage_log::principal_log_add_link(
  const service_provider & sp,
  const guid & source_id,
  const guid & target_id)
{
  principal_log_link_table t;
  
  database_transaction::current(sp)
  .insert_into(t)
  .set(t.parent_id = source_id, t.follower_id = target_id)
  .execute();
}

void vds::_storage_log::add_principal_log(
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

void vds::_storage_log::principal_log_update_state(
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
vds::_storage_log::principal_log_get_state(
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

size_t vds::_storage_log::get_current_state(const service_provider & sp, std::list<guid>& active_records)
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

void vds::_storage_log::principal_log_get_parents(
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

void vds::_storage_log::principal_log_get_followers(
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

void vds::_storage_log::processed_record(
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

uint64_t vds::_storage_log::get_principal_log_max_index(
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

void vds::_storage_log::get_unknown_records(
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

bool vds::_storage_log::get_record(
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

bool vds::_storage_log::get_front_record(
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

void vds::_storage_log::delete_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  throw std::runtime_error("Not implemented");
}

bool vds::_storage_log::get_record_by_state(
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


void vds::_storage_log::get_principal_log(
  const service_provider & sp,
  const guid & principal_id,
  size_t last_order_num,
  size_t & result_last_order_num,
  std::list<principal_log_record> & records)
{
  size_t count = 0;
  result_last_order_num = last_order_num;

  this->get_principal_log_query_.query(
    this->db_,
    "SELECT order_num,id,message\
     FROM principal_log\
     WHERE principal_id=@principal_id AND order_num<=@order_num\
     ORDER BY order_num DESC",
    [sp, &count, &result_last_order_num, principal_id, &records](sql_statement & st)->bool {

      size_t order_num;
      principal_log_record::record_id id;
      std::string message;

      st.get_value(0, order_num);

      if (result_last_order_num != order_num) {
        result_last_order_num = order_num;

        if (10 < count) {
          return false;
        }
      }

      st.get_value(1, id);
      st.get_value(2, message);

      std::shared_ptr<json_value> msg;
      dataflow(
        dataflow_arguments<char>(message.c_str(), message.length()),
        json_parser("Message parser"),
        dataflow_require_once<std::shared_ptr<json_value>>(&msg))(
          [](const service_provider & sp) {
          },
          [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            throw *ex;
          },
          sp);

      records.push_back(principal_log_record(
        id,
        principal_id,
        std::list<principal_log_record::record_id>(),
        msg,
        order_num));

      ++count;
      return true;
    },
    principal_id,
    last_order_num);
}

vds::principal_log_record
  vds::_storage_log::add_local_record(
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

bool vds::_storage_log::save_record(
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
