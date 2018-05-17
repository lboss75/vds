/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/data_coin_balance.h"
#include "private/transaction_state_calculator.h"

vds::transactions::data_coin_balance::data_coin_balance(vds::transactions::data_coin_balance &&original)
: state_(std::move(original.state_)) {
}

vds::transactions::data_coin_balance vds::transactions::data_coin_balance::load(
    vds::database_transaction &t) {

  uint64_t max_order_no = 0;
  std::set<vds::const_data_buffer> base_packages;

  vds::orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
    t1.select(t1.id, t1.order_no, t1.state_data)
    .where(t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while (st.execute()) {
    base_packages.emplace(t1.id.get(st));
    auto order = t1.order_no.get(st);
    if (max_order_no < order) {
      max_order_no = order;
    }
  }

  if(base_packages.empty()) {
    throw std::runtime_error("Invalid state");
  }

  return vds::transactions::data_coin_balance(
    max_order_no + 1,
    transaction_state_calculator::calculate(t, base_packages, max_order_no));
}

