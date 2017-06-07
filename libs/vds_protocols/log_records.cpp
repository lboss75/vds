/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"
#include "storage_object_id.h"

const char vds::server_log_record::message_type[] = "server log";

vds::server_log_record::server_log_record()
{
}

vds::server_log_record::server_log_record(const server_log_record & origin)
: id_(origin.id_),
  parents_(origin.parents_),
  message_(origin.message_)
{
}

vds::binary_deserializer & vds::server_log_record::deserialize(
  const service_provider & sp,
  binary_deserializer & b)
{
  b >> this->id_.source_id >> this->id_.index;

  auto parent_count = b.read_number();
  for (decltype(parent_count) i = 0; i < parent_count; ++i) {
    server_log_record::record_id item;
    b >> item.source_id >> item.index;
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
      [](const service_provider & sp, std::exception_ptr ex) { std::rethrow_exception(ex);},
    sp);

  return b;
}

vds::server_log_record::server_log_record(
  const record_id & id,
  const std::list<record_id> & parents,
  const std::shared_ptr<json_value> & message)
: id_(id),
  parents_(parents),
  message_(message)
{
}

vds::server_log_record::server_log_record(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("s", this->id_.source_id);
    s->get_property("i", this->id_.index);

    this->message_ = s->get_property("m");

    auto m = std::dynamic_pointer_cast<json_array>(s->get_property("p"));
    if(m) {
      for (size_t i = 0; i < m->size(); ++i) {
        auto item = std::dynamic_pointer_cast<json_object>(m->get(i));
        if (nullptr != item) {
          guid source_id;
          if (!item->get_property("s", this->id_.source_id, false)) {
            source_id = this->id_.source_id;
          }

          uint64_t index;
          item->get_property("i", index);

          this->parents_.push_back(record_id{ source_id, index });
        }
      }
    }
  }
}

void vds::server_log_record::reset(
  const record_id & id,
  const std::list<record_id>& parents,
  const std::shared_ptr<json_value> & message)
{
  this->id_ = id;
  this->parents_ = parents;
  this->message_ = message;
}

void vds::server_log_record::add_parent(
  const guid & source_id,
  uint64_t index)
{
  this->parents_.push_back(record_id { source_id, index });
}

void vds::server_log_record::serialize(binary_serializer & b) const
{
  b << this->id_.source_id << this->id_.index;

  b.write_number(this->parents_.size());
  for (auto & p : this->parents_) {
    b << p.source_id << p.index;
  }

  b << this->message_->str();
}


std::shared_ptr<vds::json_value> vds::server_log_record::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  result->add_property("s", this->id_.source_id);
  result->add_property("i", this->id_.index);
  result->add_property(std::make_shared<json_property>("m", this->message_));

  if (!this->parents_.empty()) {
    auto parents = std::make_shared<json_array>();
    for (auto& p : this->parents_) {
      std::shared_ptr<json_object> item(new json_object());
      if (p.source_id == this->id_.source_id) {
        item->add_property("s", p.source_id);
      }
      item->add_property("i", p.index);
      parents->add(item);
    }

    result->add_property(std::make_shared<json_property>("p", parents));
  }
  
  return std::shared_ptr<vds::json_value>(result.release());
}
////////////////////////////////////////////////////////////////////////
const char vds::server_log_new_object::message_type[] = "new object";

vds::server_log_new_object::server_log_new_object(
  uint64_t index,
  uint32_t lenght,
  const const_data_buffer & hash,
  const guid & owner_principal)
: index_(index),
  lenght_(lenght),
  hash_(hash),
  owner_principal_(owner_principal)
{
}

vds::server_log_new_object::server_log_new_object(
  const std::shared_ptr<vds::json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (nullptr != s) {
    s->get_property("i", this->index_);
    s->get_property("l", this->lenght_);
    s->get_property("h", this->hash_);
    s->get_property("o", this->owner_principal_);
  }
}

std::shared_ptr<vds::json_value> vds::server_log_new_object::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }  
  
  result->add_property("i", this->index_);
  result->add_property("l", this->lenght_);
  result->add_property("h", this->hash_);
  result->add_property("o", this->owner_principal_);

  return std::shared_ptr<vds::json_value>(result.release());
}



////////////////////////////////////////////////////////////////////////
const char vds::server_log_root_certificate::message_type[] = "root";

vds::server_log_root_certificate::server_log_root_certificate(
  const std::string & user_cert,
  const std::string & user_private_key,
  const const_data_buffer & password_hash)
  : user_cert_(user_cert),
  user_private_key_(user_private_key),
  password_hash_(password_hash)
{
}

vds::server_log_root_certificate::server_log_root_certificate(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("c", this->user_cert_);
    s->get_property("k", this->user_private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::shared_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("c", this->user_cert_);
  result->add_property("k", this->user_private_key_);
  result->add_property("h", this->password_hash_);
  
  return std::shared_ptr<vds::json_value>(result.release());
}

const char vds::server_log_new_user_certificate::message_type[] = "certificate";

std::shared_ptr<vds::json_value> vds::server_log_new_user_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("u", this->user_cert_);
  
  return std::shared_ptr<vds::json_value>(result.release());
}

vds::server_log_new_user_certificate::server_log_new_user_certificate(uint64_t user_cert)
  : user_cert_(user_cert)
{
}

vds::server_log_new_user_certificate::server_log_new_user_certificate(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if(s) {
    s->get_property("u", this->user_cert_);
  }
}

const char vds::server_log_batch::message_type[] = "batch";

vds::server_log_batch::server_log_batch()
 : messages_(new json_array())
{
}

vds::server_log_batch::server_log_batch(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (nullptr != s) {
     auto m = s->get_property("m");
    auto ma = std::dynamic_pointer_cast<json_array>(m);
    if(ma){
      this->messages_ = ma;
    }
  }
}

void vds::server_log_batch::add(const std::shared_ptr<json_value> & item)
{
  this->messages_->add(item);
}

std::shared_ptr<vds::json_value> vds::server_log_batch::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property(std::make_shared<json_property>("m", this->messages_));

  return std::shared_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_server::message_type[] = "new server";

vds::server_log_new_server::server_log_new_server(
  const std::string & certificate)
: certificate_(certificate)
{
}


vds::server_log_new_server::server_log_new_server(
  const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("c", this->certificate_);
  }
}


std::shared_ptr<vds::json_value> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_);
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

//////////////////////////////////////////////////////////////////////
vds::server_log_sign::server_log_sign(
  const std::string & subject,
  const const_data_buffer & signature)
  : subject_(subject), signature_(signature)
{
}

vds::server_log_sign::server_log_sign(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("n", this->subject_);
    s->get_property("s", this->signature_);
  }
}

std::shared_ptr<vds::json_value> vds::server_log_sign::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("n", this->subject_);
  result->add_property("s", this->signature_);
  return std::shared_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_file_map::message_type[] = "file";

vds::server_log_file_map::server_log_file_map(
  const guid & server_id,
  const std::string & version_id,
  const std::string & user_login,
  const std::string & name,
  const const_data_buffer & meta_info)
: server_id_(server_id),
  version_id_(version_id),
  user_login_(user_login),
  name_(name),
  meta_info_(meta_info)
{
}

vds::server_log_file_map::server_log_file_map(const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("s", this->server_id_);
    s->get_property("v", this->version_id_);
    s->get_property("u", this->user_login_);
    s->get_property("n", this->name_);
    s->get_property("m", this->meta_info_);

    auto items = std::dynamic_pointer_cast<json_array>(s->get_property("i"));
    if (items) {
      for (size_t i = 0; i < items->size(); ++i) {
        this->add(server_log_new_object(items->get(i)));
      }
    }
  }
}

void vds::server_log_file_map::add(const server_log_new_object & item)
{
  this->items_.push_back(item);
}

std::shared_ptr<vds::json_value> vds::server_log_file_map::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  
  result->add_property("s", this->server_id_);
  result->add_property("v", this->version_id_);
  result->add_property("u", this->user_login_);
  result->add_property("n", this->name_);
  result->add_property("m", this->meta_info_);

  auto items = std::make_shared<json_array>();
  for (auto & p : this->items_) {
    items->add(p.serialize(false));
  }
  result->add_property(std::make_shared<json_property>("i", items));

  return std::shared_ptr<vds::json_value>(result.release());
}

vds::server_log_file_version::server_log_file_version(
  const std::string & version_id,
  const guid & server_id)
: version_id_(version_id),
  server_id_(server_id)
{
}
