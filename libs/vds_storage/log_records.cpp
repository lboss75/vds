/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "log_records.h"

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

void vds::server_log_record::add_signature(const std::string & fingerprint, const std::string & signature)
{
  this->signatures_.push_back(server_log_sign(fingerprint, signature));
}

std::unique_ptr<vds::json_value> vds::server_log_record::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property(new json_property("m", this->message_->serialize().release()));

  std::unique_ptr<json_array> signatures(new json_array());

  for (auto& m : this->signatures_) {
    signatures->add(m.serialize());
  }

  result->add_property(new json_property("s", signatures.release()));
  
  return std::unique_ptr<vds::json_value>(result.release());
}

const char vds::server_log_root_certificate::message_type[] = "root";

vds::server_log_root_certificate::server_log_root_certificate(
  const std::string & certificate,
  const std::string & private_key,
  const std::string & password_hash)
  : certificate_(certificate),
  private_key_(private_key),
  password_hash_(password_hash)
{
}

vds::server_log_root_certificate::server_log_root_certificate(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("k", this->private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_root_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  result->add_property("h", this->password_hash_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

const char vds::server_log_new_user_certificate::message_type[] = "certificate";

std::unique_ptr<vds::json_value> vds::server_log_new_user_certificate::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("$t", message_type);
  result->add_property("c", this->certificate_);
  result->add_property("k", this->private_key_);
  
  return std::unique_ptr<vds::json_value>(result.release());
}

vds::server_log_new_user_certificate::server_log_new_user_certificate(const vds::json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if(nullptr != s) {
    s->get_property("c", this->certificate_);
    s->get_property("k", this->private_key_);
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
  const std::string& certificate)
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
  const std::string & addresses)
  : addresses_(addresses)
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
vds::server_log_sign::server_log_sign(const std::string & fingerprint, const std::string & signature)
  : fingerprint_(fingerprint), signature_(signature)
{
}

vds::server_log_sign::server_log_sign(const json_value * source)
{
  auto s = dynamic_cast<const json_object *>(source);
  if (nullptr != s) {
    s->get_property("f", this->fingerprint_);
    s->get_property("s", this->signature_);
  }
}

std::unique_ptr<vds::json_value> vds::server_log_sign::serialize() const
{
  std::unique_ptr<json_object> result(new json_object());
  result->add_property("f", this->fingerprint_);
  result->add_property("s", this->signature_);
  return std::unique_ptr<vds::json_value>(result.release());
}

