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

vds::expected<vds::const_data_buffer> vds::transactions::transaction_block_builder::save(
  const service_provider * sp,
  class vds::database_transaction &t,
  const std::shared_ptr<certificate> &write_cert,
  const std::shared_ptr<asymmetric_private_key> &write_private_key) {

  GET_EXPECTED(data, sign(
    sp,
    write_cert,
    write_private_key));

  CHECK_EXPECTED(transaction_log::save(sp, t, data));

  return hash::signature(hash::sha256(), data);
}

vds::expected<vds::const_data_buffer> vds::transactions::transaction_block_builder::sign(
  const service_provider * /*sp*/,
  const std::shared_ptr<certificate> &write_cert,
  const std::shared_ptr<asymmetric_private_key> &write_private_key) {
  vds_assert(0 != this->data_.size());
  binary_serializer block_data;
  CHECK_EXPECTED(serialize(block_data, transaction_block::CURRENT_VERSION));
  CHECK_EXPECTED(serialize(block_data, static_cast<uint64_t>(std::chrono::system_clock::to_time_t(this->time_point_))));
  CHECK_EXPECTED(serialize(block_data, this->order_no_));
  CHECK_EXPECTED(serialize(block_data, write_cert->subject()));
  CHECK_EXPECTED(serialize(block_data, this->ancestors_));
  CHECK_EXPECTED(serialize(block_data, this->data_.move_data()));

  GET_EXPECTED(sig_data, asymmetric_sign::signature(
    hash::sha256(),
    *write_private_key,
    block_data.get_buffer(),
    block_data.size()));

  CHECK_EXPECTED(serialize(block_data, sig_data));

  return block_data.move_data();
}

vds::transactions::transaction_block_builder::transaction_block_builder(const service_provider * sp)
  : sp_(sp), time_point_(std::chrono::system_clock::now()), order_no_(1){
}

vds::expected<vds::transactions::transaction_block_builder> vds::transactions::transaction_block_builder::create(
  const service_provider * sp,
  vds::database_read_transaction& t) {

  orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st, t.get_reader(
    t1.select(t1.id, t1.order_no)
    .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf)));
  std::set<const_data_buffer> ancestors;
  uint64_t order_no = 0;
  WHILE_EXPECTED(st.execute())
    ancestors.emplace(t1.id.get(st));
    auto order = safe_cast<uint64_t>(t1.order_no.get(st));
    if (order_no < order) {
      order_no = order;
    }
  WHILE_EXPECTED_END()

  ++order_no;
  return expected<transaction_block_builder>(
    sp,
    std::chrono::system_clock::now(),
    ancestors,
    order_no);
}

vds::expected<vds::transactions::transaction_block_builder> vds::transactions::transaction_block_builder::create(
  const service_provider * sp,
  vds::database_read_transaction& t,
  const std::set<const_data_buffer> & ancestors){

  uint64_t order_no = 0;

  for (const auto & ancestor : ancestors) {
    orm::transaction_log_record_dbo t1;
    GET_EXPECTED(st, t.get_reader(
      t1.select(t1.order_no)
      .where(t1.id == ancestor)));
    WHILE_EXPECTED(st.execute())
      auto order = safe_cast<uint64_t>(t1.order_no.get(st));
      if (order_no < order) {
        order_no = order;
      }
    WHILE_EXPECTED_END()
  }

  ++order_no;
  return expected<transaction_block_builder>(sp,
    std::chrono::system_clock::now(),
    ancestors,
    order_no);
}


vds::expected<void> vds::transactions::transaction_block_builder::add(expected<root_user_transaction> && item) {
  if (item.has_error()) {
    return unexpected(std::move(item.error()));
  }

  CHECK_EXPECTED(serialize(this->data_, (uint8_t)root_user_transaction::message_id));
  _serialize_visitor v(this->data_);
  const_cast<root_user_transaction &>(item.value()).visit(v);

  if (v.error()) {
    return unexpected(std::move(v.error()));
  }

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_block_builder::add(expected<create_user_transaction> && item) {
  if(item.has_error()) {
    return unexpected(std::move(item.error()));
  }

  CHECK_EXPECTED(serialize(this->data_, (uint8_t)create_user_transaction::message_id));
  _serialize_visitor v(this->data_);
  const_cast<create_user_transaction &>(item.value()).visit(v);
  if(v.error()) {
    return unexpected(std::move(v.error()));
  }

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_block_builder::add(expected<payment_transaction> && item) {
  if (item.has_error()) {
    return unexpected(std::move(item.error()));
  }

  CHECK_EXPECTED(serialize(this->data_, (uint8_t)payment_transaction::message_id));
  _serialize_visitor v(this->data_);
  const_cast<payment_transaction &>(item.value()).visit(v);

  if (v.error()) {
    return unexpected(std::move(v.error()));
  }

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_block_builder::add(
  expected<vds::transactions::channel_message> && item) {
  if (item.has_error()) {
    return unexpected(std::move(item.error()));
  }

  CHECK_EXPECTED(serialize(this->data_, (uint8_t)channel_message::message_id));
  _serialize_visitor v(this->data_);
  const_cast<channel_message &>(item.value()).visit(v);

  if (v.error()) {
    return unexpected(std::move(v.error()));
  }

  return expected<void>();
}

