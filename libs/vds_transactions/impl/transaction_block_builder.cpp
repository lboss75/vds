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

vds::const_data_buffer vds::transactions::transaction_block_builder::save(
  const service_provider *sp,
  class vds::database_transaction &t,
  const std::shared_ptr<certificate> &write_cert,
  const std::shared_ptr<asymmetric_private_key> &write_private_key) {
  vds_assert(0 != this->data_.size());
  binary_serializer block_data;
  block_data
    << transaction_block::CURRENT_VERSION
    << static_cast<uint64_t>(std::chrono::system_clock::to_time_t(this->time_point_))
    << (this->ancestors_.empty() ? 1 : this->balance_.order_no())
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
  if(this->ancestors_.empty()) {
    //Root transaction
    transaction_block block(data);
    block.walk_messages(
      [this, id](const root_user_transaction & message)->bool{
      this->balance_ = data_coin_balance(id, message.user_cert->subject());
      return true;
    });

  }

  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
    t2.id = id,
    t2.data = data,
    t2.state = orm::transaction_log_record_dbo::state_t::leaf,
    t2.order_no = this->balance_.order_no(),
    t2.time_point = this->time_point_,
    t2.state_data = this->balance_.serialize()));

  for (auto & ancestor : this->ancestors_) {
    orm::transaction_log_record_dbo t1;
    t.execute(
      t1.update(t1.state = orm::transaction_log_record_dbo::state_t::processed)
      .where(t1.id == ancestor));
  }

  return id;
}

vds::transactions::transaction_block_builder::transaction_block_builder()
  : time_point_(std::chrono::system_clock::now()) {
}

vds::transactions::transaction_block_builder::transaction_block_builder(
  const service_provider * sp,
  vds::database_transaction& t)
: time_point_(std::chrono::system_clock::now()),
  balance_(data_coin_balance::load(t, this->time_point_, this->ancestors_)) {
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

