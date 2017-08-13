/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"
#include "storage_object_id.h"
#include "chunk_manager_p.h"

////////////////////////////////////////////////////////////////////////
const char vds::principal_log_new_object::message_type[] = "new object";

vds::principal_log_new_object::principal_log_new_object(
  const guid & index,
  uint32_t lenght,
  const const_data_buffer & meta_data)
: index_(index),
  lenght_(lenght),
  meta_data_(meta_data)  
{
}

vds::principal_log_new_object::principal_log_new_object(
  const std::shared_ptr<vds::json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (nullptr != s) {
    s->get_property("i", this->index_);
    s->get_property("l", this->lenght_);
    s->get_property("m", this->meta_data_);
  }
}

std::shared_ptr<vds::json_value> vds::principal_log_new_object::serialize(bool add_type_property) const
{
  auto result = std::make_shared<json_object>();
  if (add_type_property) {
    result->add_property("$t", message_type);
  }  
  
  result->add_property("i", this->index_);
  result->add_property("l", this->lenght_);
  result->add_property("m", this->meta_data_);

  return result;
}
///////////////////////////////////////////////////////////////////////
const char vds::principal_log_record::message_type[] = "server log";

vds::principal_log_record::principal_log_record()
{
}

vds::principal_log_record::principal_log_record(const principal_log_record & origin)
: id_(origin.id_),
  principal_id_(origin.principal_id_),
  parents_(origin.parents_),
  message_(origin.message_),
  order_num_(origin.order_num_)
{
}

vds::binary_deserializer & vds::principal_log_record::deserialize(
  const service_provider & sp,
  binary_deserializer & b)
{
  b >> this->id_ >> this->principal_id_ >> this->order_num_;

  auto parent_count = b.read_number();
  for (decltype(parent_count) i = 0; i < parent_count; ++i) {
    principal_log_record::record_id item;
    b >> item;
    this->parents_.push_back(item);
  }

  std::string message;
  b >> message;

  std::shared_ptr<json_value> body;
  dataflow(
    dataflow_arguments<char>(message.c_str(), message.length()),
    json_parser("Message"),
    dataflow_require_once<std::shared_ptr<json_value>>(&body))(
      [this, &body](const service_provider & sp) { this->message_ = body; },
      [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        std::rethrow_exception(std::make_exception_ptr(*ex));
      },
      sp);

  return b;
}

vds::principal_log_record::principal_log_record(
  const record_id & id,
  const guid & principal_id,
  const std::list<record_id> & parents,
  const std::shared_ptr<json_value> & message,
  size_t order_num)
: id_(id),
  principal_id_(principal_id),
  parents_(parents),
  message_(message),
  order_num_(order_num)
{
}

vds::principal_log_record::principal_log_record(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    
    s->get_property("i", this->id_);
    s->get_property("p", this->principal_id_);
    s->get_property("o", this->order_num_);
    
    this->message_ = s->get_property("m");

    auto m = std::dynamic_pointer_cast<json_array>(s->get_property("d"));
    if(m) {
      for (size_t i = 0; i < m->size(); ++i) {
        auto item = std::dynamic_pointer_cast<json_primitive>(m->get(i));
        if (nullptr != item) {
          guid source_id = guid::parse(item->value());
          this->parents_.push_back(source_id);
        }
      }
    }
  }
}

void vds::principal_log_record::reset(
  const record_id & id,
  const guid & principal_id,
  const std::list<record_id>& parents,
  const std::shared_ptr<json_value> & message,
  size_t order_num)
{
  this->id_ = id;
  this->principal_id_ = principal_id;
  this->parents_ = parents;
  this->message_ = message;
  this->order_num_ = order_num;
}

void vds::principal_log_record::add_parent(
  const guid & index)
{
  this->parents_.push_back(index);
}

void vds::principal_log_record::serialize(binary_serializer & b) const
{
  b << this->id_ << this->principal_id_ << this->order_num_;

  b.write_number(this->parents_.size());
  for (auto & p : this->parents_) {
    b << p;
  }

  b << this->message_->str();
}


std::shared_ptr<vds::json_value> vds::principal_log_record::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  result->add_property("i", this->id_);
  result->add_property("p", this->principal_id_);
  result->add_property("o", this->order_num_);
  result->add_property(std::make_shared<json_property>("m", this->message_));

  if (!this->parents_.empty()) {
    auto parents = std::make_shared<json_array>();
    for (auto& p : this->parents_) {
      std::shared_ptr<json_value> item(new json_primitive(p.str()));
      parents->add(item);
    }

    result->add_property(std::make_shared<json_property>("d", parents));
  }
  
  return std::shared_ptr<vds::json_value>(result.release());
}
////////////////////////////////////////////////////////////////////////
// const char vds::principal_log_new_object_map::message_type[] = "new object map";
// 
// vds::principal_log_new_object_map::principal_log_new_object_map(
//   const std::shared_ptr<vds::json_value> & source)
// {
//   auto s = std::dynamic_pointer_cast<json_object>(source);
//   if (nullptr != s) {
//     s->get_property("i", this->server_id_);
//     s->get_property("o", this->object_id_);
//     s->get_property("l", this->length_);
//     s->get_property("h", this->hash_);
//     s->get_property("s", this->min_chunk_);
//     s->get_property("f", this->max_chunk_);
//   }
// }
// 
// std::shared_ptr<vds::json_value> vds::principal_log_new_object_map::serialize(bool add_type_property) const
// {
//   auto result = std::make_shared<json_object>();
//   if (add_type_property) {
//     result->add_property("$t", message_type);
//   }  
//   
//   result->add_property("i", this->server_id_);
//   result->add_property("o", this->object_id_);
//   result->add_property("l", this->length_);
//   result->add_property("h", this->hash_);
//   result->add_property("s", this->min_chunk_);
//   result->add_property("f", this->max_chunk_);
// 
//   return result;
// }
////////////////////////////////////////////////////////////////////////
const char vds::server_log_root_certificate::message_type[] = "root";

vds::server_log_root_certificate::server_log_root_certificate(
  const guid & id,
  const std::string & user_cert,
  const std::string & user_private_key,
  const const_data_buffer & password_hash)
: id_(id),
  user_cert_(user_cert),
  user_private_key_(user_private_key),
  password_hash_(password_hash)
{
}

vds::server_log_root_certificate::server_log_root_certificate(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("i", this->id_);
    s->get_property("c", this->user_cert_);
    s->get_property("k", this->user_private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::shared_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("i", this->id_);
  result->add_property("c", this->user_cert_);
  result->add_property("k", this->user_private_key_);
  result->add_property("h", this->password_hash_);
  
  return std::shared_ptr<vds::json_value>(result.release());
}

//const char vds::server_log_new_user_certificate::message_type[] = "certificate";
//
//std::shared_ptr<vds::json_value> vds::server_log_new_user_certificate::serialize() const
//{
//  std::unique_ptr<json_object> result(new json_object());
//  result->add_property("$t", message_type);
//  result->add_property("u", this->user_cert_);
//  
//  return std::shared_ptr<vds::json_value>(result.release());
//}
//
//vds::server_log_new_user_certificate::server_log_new_user_certificate(uint64_t user_cert)
//  : user_cert_(user_cert)
//{
//}
//
//vds::server_log_new_user_certificate::server_log_new_user_certificate(const std::shared_ptr<json_value> & source)
//{
//  auto s = std::dynamic_pointer_cast<json_object>(source);
//  if(s) {
//    s->get_property("u", this->user_cert_);
//  }
//}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_server::message_type[] = "new server";

vds::server_log_new_server::server_log_new_server(
  const guid & id,
  const guid & parent_id,
  const std::string & server_cert,
  const std::string & server_private_key,
  const const_data_buffer & password_hash)
: id_(id),
  parent_id_(parent_id),
  server_cert_(server_cert),
  server_private_key_(server_private_key),
  password_hash_(password_hash)
{
}


vds::server_log_new_server::server_log_new_server(
  const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("i", this->id_);
    s->get_property("p", this->parent_id_);
    s->get_property("c", this->server_cert_);
    s->get_property("k", this->server_private_key_);
    s->get_property("h", this->password_hash_);
  }
}


std::shared_ptr<vds::json_value> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("i", this->id_);
  result->add_property("p", this->parent_id_);
  result->add_property("c", this->server_cert_);
  result->add_property("k", this->server_private_key_);
  result->add_property("h", this->password_hash_);
  return std::shared_ptr<vds::json_value>(result.release());
}
//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_endpoint::message_type[] = "new endpoint";

vds::server_log_new_endpoint::server_log_new_endpoint(
  const guid & server_id,
  const std::string & addresses)
  : server_id_(server_id), addresses_(addresses)
{
}


vds::server_log_new_endpoint::server_log_new_endpoint(
  const std::shared_ptr<vds::json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("s", this->server_id_);
    s->get_property("a", this->addresses_);
  }
}


std::shared_ptr<vds::json_value> vds::server_log_new_endpoint::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("s", this->server_id_);
  result->add_property("a", this->addresses_);
  return std::shared_ptr<vds::json_value>(result.release());
}
/////////////////////////////////////////////////////////////////
const char vds::principal_log_new_chunk::message_type[] = "new chunk";

vds::principal_log_new_chunk::principal_log_new_chunk(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("s", this->server_id_);
    s->get_property("o", this->object_id_);
    s->get_property("l", this->size_);
    s->get_property("i", this->chunk_index_);
    s->get_property("h", this->hash_);
  }
}

std::shared_ptr<vds::json_value> vds::principal_log_new_chunk::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("s", this->server_id_);
  result->add_property("o", this->object_id_);
  result->add_property("l", this->size_);
  result->add_property("i", this->chunk_index_);
  result->add_property("h", this->hash_);
  return result;

}
/////////////////////////////////////////////////////////////////
const char vds::principal_log_new_replica::message_type[] = "new replica";

vds::principal_log_new_replica::principal_log_new_replica(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("s", this->server_id_);
    s->get_property("o", this->object_id_);
    s->get_property("i", this->chunk_index_);
    s->get_property("l", this->replica_size_);
    s->get_property("h", this->replica_hash_);
    s->get_property("r", this->index_);
  }
}

std::shared_ptr<vds::json_value> vds::principal_log_new_replica::serialize() const
{
  auto result = std::make_shared<json_object>();
  result->add_property("$t", message_type);
  result->add_property("s", this->server_id_);
  result->add_property("o", this->object_id_);
  result->add_property("i", this->chunk_index_);
  result->add_property("l", this->replica_size_);
  result->add_property("h", this->replica_hash_);
  result->add_property("r", this->index_);
  return result;

}

/////////////////////////////////////////////////////
size_t vds::principal_log_new_chunk::replica_size() const
{
  return 2 * (this->size_ / (2 * _chunk_manager::MIN_HORCRUX)) + ((0 == (this->size_ % (2 * _chunk_manager::MIN_HORCRUX))) ? 0 : 2); 
}
