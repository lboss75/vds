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
  message_(origin.message_->clone())
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

  dataflow(
    json_parser("Message"),
    json_require_once())(
      [this](const service_provider & sp, json_value * body) {
        this->message_.reset(body);
      },
      [](const service_provider & sp, std::exception_ptr ex) {},
    sp,
    message.c_str(),
    message.length());

  return b;
}


vds::server_log_record::server_log_record(
  const record_id & id,
  const std::list<record_id> & parents,
  const json_value * message)
: id_(id),
  parents_(parents),
  message_(message->clone())
{
}

vds::server_log_record::server_log_record(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("s", this->id_.source_id);
    s->get_property("i", this->id_.index);

    this->message_ = s->get_property("m")->clone();

    auto m = dynamic_cast<const json_array *>(s->get_property("p"));
    if(nullptr != m) {
      for (size_t i = 0; i < m->size(); ++i) {
        auto item = dynamic_cast<const json_object *>(m->get(i));
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
  json_value * message)
{
  this->id_ = id;
  this->parents_ = parents;
  this->message_.reset(message);
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


std::unique_ptr<vds::json_value> vds::server_log_record::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  result->add_property("s", this->id_.source_id);
  result->add_property("i", this->id_.index);
  result->add_property(new json_property("m", this->message_->clone().release()));

  if (!this->parents_.empty()) {
    std::unique_ptr<json_array> parents(new json_array());
    for (auto& p : this->parents_) {
      std::unique_ptr<json_object> item(new json_object());
      if (p.source_id == this->id_.source_id) {
        item->add_property("s", p.source_id);
      }
      item->add_property("i", p.index);
      parents->add(item.release());
    }

    result->add_property(new json_property("p", parents.release()));
  }
  
  return std::unique_ptr<vds::json_value>(result.release());
}
////////////////////////////////////////////////////////////////////////
const char vds::server_log_new_object::message_type[] = "new object";

vds::server_log_new_object::server_log_new_object(
  uint64_t index,
  uint32_t original_lenght,
  const vds::const_data_buffer & original_hash,
  uint32_t target_lenght,
  const vds::const_data_buffer& target_hash)
: index_(index),
  original_lenght_(original_lenght),
  original_hash_(original_hash),
  target_lenght_(target_lenght),
  target_hash_(target_hash)
{
}

vds::server_log_new_object::server_log_new_object(
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("i", this->index_);
    s->get_property("l", this->original_lenght_);
    s->get_property("h", this->original_hash_);
    s->get_property("tl", this->target_lenght_);
    s->get_property("th", this->target_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_new_object::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }  
  
  result->add_property("i", this->index_);
  result->add_property("l", this->original_lenght_);
  result->add_property("h", this->original_hash_);
  result->add_property("tl", this->target_lenght_);
  result->add_property("th", this->target_hash_);

  return std::unique_ptr<vds::json_value>(result.release());
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

vds::server_log_root_certificate::server_log_root_certificate(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->user_cert_);
    s->get_property("k", this->user_private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("c", this->user_cert_);
  result->add_property("k", this->user_private_key_);
  result->add_property("h", this->password_hash_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

const char vds::server_log_new_user_certificate::message_type[] = "certificate";

std::unique_ptr<vds::json_value> vds::server_log_new_user_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("u", this->user_cert_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

vds::server_log_new_user_certificate::server_log_new_user_certificate(uint64_t user_cert)
  : user_cert_(user_cert)
{
}

vds::server_log_new_user_certificate::server_log_new_user_certificate(const vds::json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if(nullptr != s) {
    s->get_property("u", this->user_cert_);
  }
}

const char vds::server_log_batch::message_type[] = "batch";

vds::server_log_batch::server_log_batch()
 : messages_(new json_array())
{
}

vds::server_log_batch::server_log_batch(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
     auto m = s->get_property("m");
    auto ma = dynamic_cast<const json_array *>(m);
    if(nullptr != ma){
      this->messages_.reset(static_cast<json_array *>(ma->clone().release()));
    }
  }
}

void vds::server_log_batch::add(std::unique_ptr<json_value>&& item)
{
  this->messages_->add(std::move(item));
}

std::unique_ptr<vds::json_value> vds::server_log_batch::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property(new json_property("m", this->messages_->clone().release()));

  return std::unique_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_server::message_type[] = "new server";

vds::server_log_new_server::server_log_new_server(
  const std::string & certificate)
: certificate_(certificate)
{
}


vds::server_log_new_server::server_log_new_server(
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->certificate_);
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_);
  return std::unique_ptr<vds::json_value>(result.release());
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
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("s", this->server_id_);
    s->get_property("a", this->addresses_);
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_endpoint::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("s", this->server_id_);
  result->add_property("a", this->addresses_);
  return std::unique_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
vds::server_log_sign::server_log_sign(
  const std::string & subject,
  const const_data_buffer & signature)
  : subject_(subject), signature_(signature)
{
}

vds::server_log_sign::server_log_sign(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("n", this->subject_);
    s->get_property("s", this->signature_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_sign::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("n", this->subject_);
  result->add_property("s", this->signature_);
  return std::unique_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_file_map::message_type[] = "file";

vds::server_log_file_map::server_log_file_map(
  const std::string & version_id,
  const std::string & user_login,
  const std::string & name)
: version_id_(version_id),
  user_login_(user_login),
  name_(name)
{
}

vds::server_log_file_map::server_log_file_map(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("v", this->version_id_);
    s->get_property("u", this->user_login_);
    s->get_property("n", this->name_);

    auto items = dynamic_cast<const json_array *>(s->get_property("i"));
    if (nullptr != items) {
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

std::unique_ptr<vds::json_value> vds::server_log_file_map::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  
  result->add_property("v", this->version_id_);
  result->add_property("u", this->user_login_);
  result->add_property("n", this->name_);

  std::unique_ptr<json_array> items(new json_array());
  for (auto & p : this->items_) {
    items->add(p.serialize(false));
  }
  result->add_property(new json_property("i", items.release()));

  return std::unique_ptr<vds::json_value>(result.release());
}

vds::server_log_file_version::server_log_file_version(
  const std::string & version_id,
  const guid & server_id)
: version_id_(version_id),
  server_id_(server_id)
{
}
