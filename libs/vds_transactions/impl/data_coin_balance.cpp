/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "private/data_coin_balance.h"
#include "orm/coin_transaction.h"
#include "coin_transaction_walker.h"

vds::data_coin_private::data_coin_balance::data_coin_balance(vds::data_coin_private::data_coin_balance &&original)
:accounts_(std::move(original.accounts_)) {
}

vds::data_coin_private::data_coin_balance vds::data_coin_private::data_coin_balance::load(
    vds::database_transaction &t,
    const std::list<vds::const_data_buffer> &base_packages) {

  auto p = base_packages.begin();
  if(base_packages.end() == p){
    return data_coin_balance();
  }

  data_coin::orm::coin_transaction t1;
  auto st = t.get_reader(
      t1.select(t1.balance, t1.order_no)
          .where(t1.id == base64::from_bytes(*p)));

  if(!st.execute()){
    throw std::runtime_error("Database is corrupted");
  }

  auto walker = coin_transaction_walker::start_with(*p, t1.order_no.get(st));

  data_coin_balance result(t1.balance.get(st));

  while(base_packages.end() != ++p){
    walker.schedule_package(
        t,
        *p,
        data_coin_private::coin_transaction_package::get_order_no(t, *p));
  }

  //
  const_data_buffer package_id;
  uint64_t order_no;
  while(walker.next_unprocessed(package_id, order_no)){
    st = t.get_reader(
        t1.select(t1.data)
            .where(t1.id == base64::from_bytes(package_id)));

    if(!st.execute()){
      throw std::runtime_error("Database is corrupted");
    }

    auto package = data_coin_private::coin_transaction_package::load(t1.data.get(st));

    for(auto t : package.transactions()){
      result.apply(package.source_user(), t);
    }
  }

  return result;
}

void vds::data_coin_private::data_coin_balance::apply(
    const const_data_buffer & source_user,
    const vds::data_coin::transactions::payment_transaction &transaction) {
  this->accounts_[source_user].balance -= transaction.value();
  this->accounts_[transaction.target_user_certificate_thumbprint()].balance += transaction.value();
}

