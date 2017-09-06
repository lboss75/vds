#include "principal_manager.h"
#include "principal_manager_p.h"
#include "json_parser.h"
#include "asymmetriccrypto.h"
#include "logger.h"

vds::principal_manager::principal_manager()
: impl_(new _principal_manager())
{
}

vds::principal_manager::~principal_manager()
{
  delete this->impl_;
}

void vds::principal_manager::lock()
{
  this->impl_->lock();
}

void vds::principal_manager::unlock()
{
  this->impl_->unlock();
}

bool vds::principal_manager::save_record(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  return this->impl_->save_record(sp, tr, record, signature);
}

std::unique_ptr<vds::principal_record> vds::principal_manager::find_principal(
  const service_provider & sp,
  database_transaction & tr,
  const guid & object_name)
{
  return this->impl_->find_principal(sp, tr, object_name);
}

std::unique_ptr<vds::principal_record> vds::principal_manager::find_user_principal(
  const service_provider & sp,
  database_transaction & tr,
  const std::string & object_name)
{
  return this->impl_->find_user_principal(sp, tr, object_name);
}

size_t vds::principal_manager::get_current_state(
  const service_provider & sp,
  database_transaction & tr,
  std::list<guid> * active_records)
{
  return this->impl_->get_current_state(sp, tr, active_records);
}

void vds::principal_manager::get_principal_log(
  const service_provider & sp,
  database_transaction & tr,
  const guid & principal_id,
  size_t last_order_num,
  size_t & result_last_order_num,
  std::list<principal_log_record> & records)
{
  this->impl_->get_principal_log(sp, tr, principal_id, last_order_num, result_last_order_num, records);
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
      cert BLOB NOT NULL,\
      key BLOB NOT NULL,\
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
  database_transaction & tr,
  const principal_record & record)
{
  principal_table t;
  
  tr.execute(
    t.insert(
      t.id = record.id(),
      t.cert = record.cert_body().der(),
      t.key = record.cert_key(),
      t.password_hash = record.password_hash(),
      t.parent = record.parent_principal()));
}

void vds::_principal_manager::add_user_principal(
  const service_provider & sp,
  database_transaction & tr,
  const std::string & login,
  const principal_record & record)
{
  this->add_principal(sp, tr, record);

  user_principal_table t;

  tr.execute(
    t.insert(
      t.id = record.id(),
      t.login = login));
}

vds::guid vds::_principal_manager::get_root_principal(
  const service_provider & sp,
  database_transaction & tr)
{
  vds::guid result;

  principal_table t;
  
  auto st = tr.get_reader(
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
  database_transaction & tr,
  const guid & object_name)
{
  std::unique_ptr<principal_record> result;
  principal_table t;
  auto st = tr.get_reader(
    t.select(t.parent,t.cert,t.key,t.password_hash).where(t.id == object_name));
  while(st.execute()){
    result.reset(new principal_record(
      t.parent.get(st),
      object_name,
      certificate::parse_der(t.cert.get(st)),
      t.key.get(st),
      t.password_hash.get(st)));
    break;
  }
  
  return result;
}

std::unique_ptr<vds::principal_record> vds::_principal_manager::find_user_principal(
  const service_provider & sp,
  database_transaction & tr,
  const std::string & object_name)
{
  std::unique_ptr<principal_record> result;
  principal_table p;
  user_principal_table u;
  auto st = tr.get_reader(
    p.select(p.parent,p.id,p.cert,p.key,p.password_hash)
    .inner_join(u, u.id == p.id)
    .where(u.login == object_name));
  
  if(st.execute()){
    result.reset(new principal_record(
      p.parent.get(st),
      p.id.get(st),
      certificate::parse_der(p.cert.get(st)),
      p.key.get(st),
      p.password_hash.get(st)));
  }
  
  return result;
}

void vds::_principal_manager::principal_log_add_link(
  const service_provider & sp,
  database_transaction & tr,
  const guid & source_id,
  const guid & target_id)
{
  principal_log_link_table t;
  
  tr.execute(
    t.insert(t.parent_id = source_id, t.follower_id = target_id));
}

void vds::_principal_manager::add_principal_log(
  const service_provider & sp,
  database_transaction & tr,
  const guid & record_id,
  const guid & principal_id,
  const std::string & body,
  const const_data_buffer & signature,
  int order_num,
  _principal_manager::principal_log_state state)
{
  principal_log_table t;
  tr.execute(
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
  database_transaction & tr,
  const principal_log_record::record_id & record_id,
  _principal_manager::principal_log_state state)
{
  principal_log_table t;
  tr.execute(t.update(t.state = (int)state).where(t.id == record_id));
}

vds::_principal_manager::principal_log_state
vds::_principal_manager::principal_log_get_state(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & record_id)
{
  auto result = principal_log_state::not_found;

  principal_log_table t;
  auto st = tr.get_reader(
    t.select(t.state).where(t.id == record_id));
  
  while(st.execute()){
    result = (principal_log_state)t.state.get(st);
  }
  
  return result;
}

size_t vds::_principal_manager::get_current_state(
  const service_provider & sp,
  database_transaction & tr,
  std::list<guid> * active_records)
{
  size_t max_order_num = 0;
  principal_log_table t;
  
  std::lock_guard<not_mutex> lock(this->principal_log_mutex_);
  
  if(nullptr != active_records){

    auto st = tr.get_reader(
      t.select(t.id, t.order_num).where(t.state == (int)principal_log_state::tail));
    while(st.execute()){
      auto order_num = t.order_num.get(st);
      if (max_order_num < order_num) {
        max_order_num = order_num;
      }

      active_records->push_back(t.id.get(st));
    }
  } else {
    auto st = tr.get_reader(
      t.select(db_max(t.order_num)).where(t.state == (int)principal_log_state::tail));
    while(st.execute()){
      st.get_value(0, max_order_num);
    }
  }
  
  return max_order_num;
}

void vds::_principal_manager::principal_log_get_parents(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & record_id,
  std::list<principal_log_record::record_id>& parents)
{
  principal_log_link_table t;

  auto st = tr.get_reader(
    t.select(t.parent_id).where(t.follower_id == record_id));

  while (st.execute()) {
    parents.push_back(t.parent_id.get(st));
  }
}

void vds::_principal_manager::principal_log_get_followers(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & record_id,
  std::list<principal_log_record::record_id>& followers)
{
  principal_log_link_table t;

  auto st = tr.get_reader(
    t.select(t.follower_id).where(t.parent_id == record_id));
  while (st.execute()) {
    followers.push_back(t.follower_id.get(st));
  }
}

void vds::_principal_manager::processed_record(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & id)
{
  std::lock_guard<not_mutex> lock(this->principal_log_mutex_);

  std::list<principal_log_record::record_id> parents;
  this->principal_log_get_parents(sp, tr, id, parents);
  for (auto& p : parents) {
    auto parent_state = this->principal_log_get_state(sp, tr, p);
    if (principal_log_state::tail == parent_state) {
      this->principal_log_update_state(sp, tr, p, principal_log_state::processed);
      break;
    }
    else if (principal_log_state::processed != parent_state) {
      throw std::runtime_error("Invalid state");
    }
  }

  std::list<principal_log_record::record_id> followers;
  this->principal_log_get_followers(sp, tr, id, followers);

  if (0 == followers.size()) {
    this->principal_log_update_state(sp, tr, id, principal_log_state::tail);
  }
  else {
    this->principal_log_update_state(sp, tr, id, principal_log_state::processed);
    
    for (auto& f : followers) {
      auto state = this->principal_log_get_state(sp, tr, f);
      switch (state) {
      case principal_log_state::stored:
      {
        std::list<principal_log_record::record_id> parents;
        this->principal_log_get_parents(sp, tr, f, parents);

        auto new_state = principal_log_state::front;
        for (auto& p : parents) {
          auto state = this->principal_log_get_state(sp, tr, p);
          if (principal_log_state::stored == state
            || principal_log_state::front == state) {
            new_state = principal_log_state::stored;
            break;
          }

          if (principal_log_state::processed != state) {
            throw std::runtime_error("Invalid state");
          }
        }

        if (state != new_state) {
          this->principal_log_update_state(sp, tr, f, new_state);
        }

        break;
      }
      default:
        throw std::runtime_error("State error");
      }
    }
  }
}

void vds::_principal_manager::get_unknown_records(
  const service_provider & sp,
  database_transaction & tr,
  std::list<principal_log_record::record_id>& result)
{
  auto st = tr.parse(
    "SELECT parent_id \
     FROM principal_log_link \
     WHERE NOT EXISTS (\
      SELECT * \
      FROM principal_log \
      WHERE principal_log.id=principal_log_link.parent_id)");

  while(st.execute()){
    principal_log_record::record_id item;
    st.get_value(0, item);

    result.push_back(item);
  }
}

bool vds::_principal_manager::get_record(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & id,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  principal_log_table t;
  auto st = tr.get_reader(
    t.select(t.principal_id, t.message, t.signature, t.order_num).where(t.id == id));

  bool result = false;
  guid principal_id;
  std::string message;
  size_t order_num;
  while (st.execute()) {
    principal_id = t.principal_id.get(st);
    message = t.message.get(st);
    result_signature = t.signature.get(st);
    order_num = t.order_num.get(st);
    result = true;
  }

  if (result) {
    std::list<principal_log_record::record_id> parents;
    this->principal_log_get_parents(sp, tr, id, parents);

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
  database_transaction & tr,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  return this->get_record_by_state(
    sp,
    tr,
    principal_log_state::front,
    result_record,
    result_signature);    
}

void vds::_principal_manager::delete_record(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record::record_id & id)
{
  throw std::runtime_error("Not implemented");
}

bool vds::_principal_manager::get_record_by_state(
  const service_provider & sp,
  database_transaction & tr,
  principal_log_state state,
  principal_log_record & result_record,
  const_data_buffer & result_signature)
{
  principal_log_table t;
  auto st = tr.get_reader(
    t.select(t.id,t.principal_id,t.message,t.order_num,t.signature)
    .where(t.state == (int)state));

  bool result = false;
  principal_log_record::record_id id;
  guid principal_id;
  std::string message;
  int order_num;
  while (st.execute()) {
    id = t.id.get(st);
    principal_id = t.principal_id.get(st);
    message = t.message.get(st);
    order_num = t.order_num.get(st);
    result_signature = t.signature.get(st);
    result = true;
    break;
  }

  if (result) {
    std::list<principal_log_record::record_id> parents;
    this->principal_log_get_parents(sp, tr, id, parents);

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
  database_transaction & tr,
  const guid & principal_id,
  size_t last_order_num,
  size_t & result_last_order_num,
  std::list<principal_log_record> & records)
{
  size_t count = 0;
  result_last_order_num = last_order_num;

  principal_log_table t;

  auto st = tr.get_reader(
    t.select(t.id, t.order_num, t.message)
    .where(t.principal_id == principal_id && t.order_num <= last_order_num)
    .order_by(db_desc_order(t.order_num)));

  while (st.execute()) {
    auto order_num = t.order_num.get(st);
    if (result_last_order_num != order_num) {
      result_last_order_num = order_num;

      if (10 < count) {
        break;
      }
    }

    principal_log_record::record_id id = t.id.get(st);
    std::string message = t.message.get(st);

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
  }
}

vds::principal_log_record
  vds::_principal_manager::add_local_record(
    const service_provider & sp,
    database_transaction & tr,
    const principal_log_record::record_id & record_id,
    const guid & principal_id,
    const std::shared_ptr<json_value> & message,
    const vds::asymmetric_private_key & principal_private_key,
    const_data_buffer & signature)
{
  std::list<principal_log_record::record_id> parents;
  auto max_order_num = this->get_current_state(sp, tr, &parents);

  std::lock_guard<not_mutex> lock(this->principal_log_mutex_);

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
    tr,
    record_id,
    principal_id,
    message->str(),
    signature,
    max_order_num + 1,
    principal_log_state::front);

  //update tails & create links
  for (auto& p : parents) {
    this->principal_log_update_state(
      sp,
      tr,
      p,
      principal_log_state::processed);

    this->principal_log_add_link(
      sp,
      tr,
      p,
      record_id);
  }

  return result;
}

bool vds::_principal_manager::save_record(
  const service_provider & sp,
  database_transaction & tr,
  const principal_log_record & record,
  const const_data_buffer & signature)
{
  std::lock_guard<not_mutex> lock(this->principal_log_mutex_);
  
  auto state = this->principal_log_get_state(sp, tr, record.id());
  if (state != principal_log_state::not_found) {
    return false;
  }

  state = principal_log_state::front;
  for (auto& p : record.parents()) {
    auto parent_state = this->principal_log_get_state(sp, tr, p);
    if(principal_log_state::not_found == parent_state
      || principal_log_state::stored == parent_state
      || principal_log_state::front == parent_state) {
      state = principal_log_state::stored;
      break;
    }
  }

  this->add_principal_log(
    sp,
    tr,
    record.id(),
    record.principal_id(),
    record.message()->str(),
    signature,
    record.order_num(),
    state);

  if (record.parents().empty()) {
    sp.get<logger>()->debug("server_log", sp, "Added record %s without parents", record.id().str().c_str());
  }
  else {
    sp.get<logger>()->debug("server_log", sp, "Added record %s", record.id().str().c_str());
  }

  for (auto& p : record.parents()) {
    this->principal_log_add_link(
      sp,
      tr,
      p,
      record.id());
    sp.get<logger>()->debug("server_log", sp, "Added parent %s", p.str().c_str());
  }

  return true;
}
