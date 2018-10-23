/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
//#include "include/data_coin_balance.h"
//#include "include/transaction_state_calculator.h"
//#include "transaction_log_record_dbo.h"
//#include "vds_debug.h"
//#include "encoding.h"
//
//vds::transactions::data_coin_balance vds::transactions::data_coin_balance::load(
//    vds::database_transaction &t,
//    const std::chrono::system_clock::time_point & time_point,
//    std::set<vds::const_data_buffer> & base_packages) {
//
//  uint64_t max_order_no = 0;
//
//  orm::transaction_log_record_dbo t1;
//  auto st = t.get_reader(
//    t1.select(t1.id, t1.order_no, t1.state_data)
//    .where(
//      t1.time_point <= time_point
//      && t1.state == orm::transaction_log_record_dbo::state_t::leaf));
//  while (st.execute()) {
//    base_packages.emplace(t1.id.get(st));
//    auto order = t1.order_no.get(st);
//    if (max_order_no < order) {
//      max_order_no = order;
//    }
//  }
//
//  if(base_packages.empty()) {
//    throw std::runtime_error("Invalid state");
//  }
//
//  return vds::transactions::data_coin_balance(
//    max_order_no + 1,
//    transaction_state_calculator::calculate(t, base_packages, time_point, max_order_no + 1));
//}
//
