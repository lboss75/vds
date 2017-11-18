/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "root_user_transaction.h"
#include "json_object.h"
#include "binary_serialize.h"

const char vds::root_user_transaction::message_type[] = "root";

vds::root_user_transaction::root_user_transaction(
    const guid & id,
    const certificate & user_cert,
    const const_data_buffer & user_private_key,
    const const_data_buffer & password_hash)
    : id_(id),
      user_cert_(user_cert),
      user_private_key_(user_private_key),
      password_hash_(password_hash)
{
}

vds::root_user_transaction::root_user_transaction(
    const std::shared_ptr<json_value> & source)
{
  auto s = std::dynamic_pointer_cast<json_object>(source);
  if (s) {
    s->get_property("i", this->id_);

    std::string user_cert;
    s->get_property("c", user_cert);
    this->user_cert_ = certificate::parse(user_cert);

    s->get_property("k", this->user_private_key_);
    s->get_property("h", this->password_hash_);
  }
}

std::shared_ptr<vds::json_value> vds::root_user_transaction::serialize(bool add_type) const
{
  std::unique_ptr<json_object> result(new json_object());
  if(add_type){
    result->add_property("$t", message_type);
  }

  result->add_property("i", this->id_);
  result->add_property("c", this->user_cert_.str());
  result->add_property("k", this->user_private_key_);
  result->add_property("h", this->password_hash_);

  return result;
}

vds::root_user_transaction::root_user_transaction(struct binary_deserializer &b) {
  const_data_buffer cert_der;
  b >> this->id_ >> cert_der >> this->user_private_key_ >> this->password_hash_;
  this->user_cert_ = certificate::parse_der(cert_der);
}

void vds::root_user_transaction::serialize(vds::binary_serializer &b) const {
  b << this->id_ << this->user_cert_.der() << this->user_private_key_ << this->password_hash_;
}
