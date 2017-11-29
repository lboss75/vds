/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "root_user_transaction.h"
#include "binary_serialize.h"

vds::root_user_transaction::root_user_transaction(
    const guid & id,
    const certificate & user_cert,
    const std::string & user_name,
    const const_data_buffer & user_private_key,
    const const_data_buffer & password_hash)
    : id_(id),
      user_cert_(user_cert),
      user_name_(user_name),
      user_private_key_(user_private_key),
      password_hash_(password_hash)
{
}

vds::root_user_transaction::root_user_transaction(struct binary_deserializer &b) {
  const_data_buffer cert_der;
  b >> this->id_ >> cert_der >> this->user_name_ >> this->user_private_key_ >> this->password_hash_;
  this->user_cert_ = certificate::parse_der(cert_der);
}

void vds::root_user_transaction::serialize(vds::binary_serializer &b) const {
  b << this->id_ << this->user_cert_.der() << this->user_name_ << this->user_private_key_ << this->password_hash_;
}
