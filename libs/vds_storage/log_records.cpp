/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"
#include "storage_object_id.h"

const char vds::server_log_record::message_type[] = "server log";

vds::server_log_record::server_log_record(std::unique_ptr<server_log_batch> && message)
: message_(std::move(message))
{
}

vds::server_log_record::server_log_record(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    this->message_.reset(new server_log_batch(s->get_property("m")));

    auto m = dynamic_cast<const json_array *>(s->get_property("s"));
    if(nullptr != m) {
      for (size_t i = 0; i < m->size(); ++i) {
        this->signatures_.push_back(server_log_sign(m->get(i)));
      }
    }
  }
}

void vds::server_log_record::add_signature(
  const std::string & subject,
  const data_buffer & signature)
{
  this->signatures_.push_back(server_log_sign(subject, signature));
}

std::unique_ptr<vds::json_value> vds::server_log_record::serialize(bool add_type_property) const
{
  std::unique_ptr<json_object> result(new json_object());
  if (add_type_property) {
    result->add_property("$t", message_type);
  }
  result->add_property(new json_property("m", this->message_->serialize().release()));

  std::unique_ptr<json_array> signatures(new json_array());

  for (auto& m : this->signatures_) {
    signatures->add(m.serialize());
  }

  result->add_property(new json_property("s", signatures.release()));
  
  return std::unique_ptr<vds::json_value>(result.release());
}
////////////////////////////////////////////////////////////////////////
const char vds::server_log_new_object::message_type[] = "new object";

vds::server_log_new_object::server_log_new_object(
  uint64_t index,
  uint32_t original_lenght,
  const vds::data_buffer & original_hash,
  uint32_t target_lenght,
  const vds::data_buffer& target_hash)
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
  const storage_object_id & user_cert,
  const data_buffer & password_hash)
  : user_cert_(user_cert),
  password_hash_(password_hash)
{
}

vds::server_log_root_certificate::server_log_root_certificate(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    this->user_cert_ = storage_object_id(s->get_property("u"));
    s->get_property("h", this->password_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("u", this->user_cert_.serialize(false));
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

vds::server_log_batch::server_log_batch(size_t message_id)
  : message_id_(message_id),
  messages_(new json_array())
{
}

vds::server_log_batch::server_log_batch(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("i", this->message_id_);

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
  result->add_property("i", std::to_string(this->message_id_));
  result->add_property(new json_property("m", this->messages_->clone().release()));

  return std::unique_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
const char vds::server_log_new_server::message_type[] = "new server";

vds::server_log_new_server::server_log_new_server(
  const storage_object_id & cert_id)
: cert_id_(cert_id)
{
}


vds::server_log_new_server::server_log_new_server(
  const vds::json_value* source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    this->cert_id_ = storage_object_id(s->get_property("c"));
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_server::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->cert_id_.serialize(false));
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
    s->get_property("a", this->addresses_);
  }
}


std::unique_ptr<vds::json_value> vds::server_log_new_endpoint::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("a", this->addresses_);
  return std::unique_ptr<vds::json_value>(result.release());
}

//////////////////////////////////////////////////////////////////////
vds::server_log_sign::server_log_sign(
  const std::string & subject,
  const data_buffer & signature)
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

vds::server_log_file_map::server_log_file_map(const std::string & user_login, const std::string & name)
  : user_login_(user_login), name_(name)
{
}

vds::server_log_file_map::server_log_file_map(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
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
  result->add_property("u", this->user_login_);
  result->add_property("n", this->name_);

  std::unique_ptr<json_array> items(new json_array());
  for (auto & p : this->items_) {
    items->add(p.serialize(false));
  }
  result->add_property(new json_property("i", items.release()));

  return std::unique_ptr<vds::json_value>(result.release());
}
