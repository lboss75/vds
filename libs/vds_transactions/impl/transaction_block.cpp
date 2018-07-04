/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "include/transaction_block.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"

bool vds::transactions::transaction_block::validate(const certificate& write_cert) {
  binary_serializer block_data;
  block_data
    << this->version_
    << (uint64_t)std::chrono::system_clock::to_time_t(this->time_point_)
    << this->order_no_
    << this->write_cert_id_
    << this->ancestors_
    << this->block_messages_;

  return asymmetric_sign_verify::verify(
    hash::sha256(),
    write_cert.public_key(),
    this->signature_, 
    block_data.data());
}

bool vds::transactions::transaction_block::exists(database_transaction& t) {
  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1
    .select(t1.state)
    .where(t1.id == base64::from_bytes(this->id())));

  if (st.execute()) {
    return true;
  }

  return false;
}
