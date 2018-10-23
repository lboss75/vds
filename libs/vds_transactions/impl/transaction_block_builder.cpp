/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "dht_network_client.h"
#include "include/transaction_block_builder.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "vds_debug.h"
#include "transactions/payment_transaction.h"
#include "transaction_block.h"
#include "create_user_transaction.h"
#include "transaction_log.h"

vds::const_data_buffer vds::transactions::transaction_block_builder::save(
  const service_provider * sp,
  class vds::database_transaction &t,
  const std::shared_ptr<certificate> &write_cert,
  const std::shared_ptr<asymmetric_private_key> &write_private_key) {
  vds_assert(0 != this->data_.size());
  binary_serializer block_data;
  block_data
    << transaction_block::CURRENT_VERSION
    << static_cast<uint64_t>(std::chrono::system_clock::to_time_t(this->time_point_))
    << this->order_no_
    << write_cert->subject()
    << this->ancestors_
    << this->data_.move_data();

  block_data << asymmetric_sign::signature(
    hash::sha256(),
    *write_private_key,
    block_data.get_buffer(),
    block_data.size());

  auto id = hash::signature(hash::sha256(), block_data.get_buffer(), block_data.size());

  const auto data = block_data.move_data();

  transaction_log::save(sp, t, data);

  return id;
}

vds::transactions::transaction_block_builder::transaction_block_builder(const service_provider * sp)
  : sp_(sp), time_point_(std::chrono::system_clock::now()), order_no_(1){
}

vds::transactions::transaction_block_builder::transaction_block_builder(
  const service_provider * sp,
  vds::database_transaction& t)
: sp_(sp), time_point_(std::chrono::system_clock::now()), order_no_(0) {

    orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(
      t1.select(t1.id, t1.order_no)
      .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf));
    while (st.execute()) {
      this->ancestors_.emplace(t1.id.get(st));
      auto order = t1.order_no.get(st);
      if (this->order_no_ < order) {
        this->order_no_ = order;
      }
    }

    this->order_no_++;
}

void vds::transactions::transaction_block_builder::add(const root_user_transaction& item) {
  this->data_ << (uint8_t)root_user_transaction::message_id;
  _serialize_visitor v(this->data_);
  const_cast<root_user_transaction &>(item).visit(v);
}

void vds::transactions::transaction_block_builder::add(const create_user_transaction& item) {
  this->data_ << (uint8_t)create_user_transaction::message_id;
  _serialize_visitor v(this->data_);
  const_cast<create_user_transaction &>(item).visit(v);
}

void vds::transactions::transaction_block_builder::add(const payment_transaction& item) {
  this->data_ << (uint8_t)payment_transaction::message_id;
  _serialize_visitor v(this->data_);
  const_cast<payment_transaction &>(item).visit(v);
}

void vds::transactions::transaction_block_builder::add(
    const vds::transactions::channel_message &item) {

  item.serialize(this->data_);

}

