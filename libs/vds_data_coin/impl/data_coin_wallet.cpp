/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "data_coin_wallet.h"
#include "private/data_coin_wallet_p.h"

void vds::data_coin_private::_wallet::merge_transaction(
  database_transaction & t,
  const data_coin::coin_transaction_package & transaction_package) {
  
  auto common_base = lookup_common_base(transaction_package, this->current_transaction_);

}

vds::data_coin::coin_transaction_package vds::data_coin_private::_wallet::lookup_common_base(
  database_transaction & t,
  const data_coin::coin_transaction_package & left_package,
  const data_coin::coin_transaction_package & right_package) {

  std::set<const_data_buffer> common_parents;
  std::set<const_data_buffer> left_parents;
  std::set<const_data_buffer> right_parents;

  left_parents.emplace(left_package.id());
  right_parents.emplace(right_package.id());



}