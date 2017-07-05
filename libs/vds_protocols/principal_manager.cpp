#include "principal_manager.h"
#include "principal_manager_p.h"

vds::principal_manager::principal_manager()
: impl_(new _principal_manager())
{
}

vds::principal_manager::~principal_manager()
{
  delete this->impl_;
}

///////////////////////////////////////////////////////
void vds::_principal_manager::create_database_objects(
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

void vds::_principal_manager::add_principal(
  const service_provider & sp,
  const principal_record & record)
{
  principal_table t;
  
  database_transaction::current(sp).execute(
    t.insert(
      t.id = record.id(),
      t.cert = record.cert_body(),
      t.key = record.cert_key(),
      t.password_hash = record.password_hash(),
      t.parent = record.parent_principal()));
}

void vds::_principal_manager::add_user_principal(
  const service_provider & sp,
  const std::string & login,
  const principal_record & record)
{
  this->add_principal(sp, record);

  user_principal_table t;

  database_transaction::current(sp).execute(
    t.insert(
      t.id = record.id(),
      t.login = login));
}

vds::guid vds::_principal_manager::get_root_principal(
  const service_provider & sp)
{
  vds::guid result;

  principal_table t;
  
  auto st = database_transaction::current(sp).get_reader(
    t.select(t.id).where(t.id == t.parent));
  
  while(st.execute()) {
    if (0 < result.size()) {
      throw std::runtime_error("Database is corrupt");
    }
    
    result = t.id.get(st);
  }

  if (0 == result.size()) {
    throw std::runtime_error("Database is corrupt");
  }

  return result;
}

std::unique_ptr<vds::principal_record> vds::_principal_manager::find_principal(
  const service_provider & sp,
  const guid & object_name)
{
  std::unique_ptr<principal_record> result;
  principal_table t;
  auto st = database_transaction::current(sp).get_reader(
    t.select(t.parent,t.cert,t.key,t.password_hash).where(t.id == object_name));
  while(st.execute()){
    result.reset(new principal_record(
      t.parent.get(st),
      object_name,
      t.cert.get(st),
      t.key.get(st),
      t.password_hash.get(st)));
    break;
  }
  
  return result;
}

std::unique_ptr<vds::principal_record> vds::_principal_manager::find_user_principal(
  const service_provider & sp,
  const std::string & object_name)
{
  std::unique_ptr<principal_record> result;
  principal_table p;
  user_principal_table u;
  auto st = database_transaction::current(sp).get_reader(
    p.select(p.parent,p.id,p.cert,p.key,p.password_hash)
    .inner_join(u, u.id == p.id)
    .where(u.login == object_name));
  
  if(st.execute()){
    result.reset(new principal_record(
      p.parent.get(st),
      p.id.get(st),
      p.cert.get(st),
      p.key.get(st),
      p.password_hash.get(st)));
  }
  
  return result;
}

void vds::_principal_manager::principal_log_add_link(
  const service_provider & sp,
  const guid & source_id,
  const guid & target_id)
{
  principal_log_link_table t;
  
  database_transaction::current(sp).execute(
    t.insert(t.parent_id = source_id, t.follower_id = target_id));
}

void vds::_principal_manager::add_principal_log(
  const service_provider & sp,
  const guid & record_id,
  const guid & principal_id,
  const std::string & body,
  const const_data_buffer & signature,
  int order_num,
  _principal_manager::principal_log_state state)
{
  principal_log_table t;
  database_transaction::current(sp).execute(
    t.insert(
      t.id = record_id,
      t.principal_id = principal_id,
      t.message = body,
      t.signature = signature,
      t.order_num = order_num,
      t.state = (int)state));
}

void vds::_principal_manager::principal_log_update_state(
  const service_provider & sp,
  const principal_log_record::record_id & record_id,
  _principal_manager::principal_log_state state)
{
  principal_log_table t;
  database_transaction::current(sp).execute(
    t.update(t.state = (int)state).where(t.id == record_id));
}

vds::_principal_manager::principal_log_state
vds::_principal_manager::principal_log_get_state(
  const service_provider & sp,
  const principal_log_record::record_id & record_id)
{
  auto result = principal_log_state::not_found;

  principal_log_table t;
  auto st = database_transaction::current(sp).get_reader(
    t.select(t.state).where(t.id == record_id));
  
  while(st.execute()){
    result = (principal_log_state)t.state.get(st);
  }
  
  return result;
}

size_t vds::_principal_manager::get_current_state(
  const service_provider & sp,
  std::list<guid> & active_records)
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

void vds::_principal_manager::principal_log_get_parents(
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

void vds::_principal_manager::principal_log_get_followers(
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

void vds::_principal_manager::processed_record(
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

uint64_t vds::_principal_manager::get_principal_log_max_index(
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

void vds::_principal_manager::get_unknown_records(
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

bool vds::_principal_manager::get_record(
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

bool vds::_principal_manager::get_front_record(
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

void vds::_principal_manager::delete_record(
  const service_provider & sp,
  const principal_log_record::record_id & id)
{
  throw std::runtime_error("Not implemented");
}

bool vds::_principal_manager::get_record_by_state(
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


void vds::_principal_manager::get_principal_log(
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
  vds::_principal_manager::add_local_record(
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

bool vds::_principal_manager::save_record(
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
