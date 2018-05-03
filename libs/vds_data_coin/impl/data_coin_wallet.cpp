/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "encoding.h"
#include "data_coin_wallet.h"
#include "private/data_coin_wallet_p.h"
#include "orm/coin_transaction.h"
#include "orm/coin_unknown_transaction.h"

void vds::data_coin_private::_wallet::save_transaction(
  database_transaction & t,
  const const_data_buffer & transaction_data) {

  auto transaction_package = data_coin_private::coin_transaction_package::parse(transaction_data);

  //Validate base packages
  auto state = data_coin::orm::coin_transaction::state_t::applied;
  auto id_str = base64::from_bytes(transaction_package.id());
  for(auto & p : transaction_package.base_packages()){
    data_coin::orm::coin_transaction t1;
    auto st = t.get_reader(t1.select(t1.state).where(t1.id == id_str));
    if(!st.execute()){
      data_coin::orm::coin_unknown_transaction t2;
      t.execute(t2.insert_or_ignore(t2.id = id_str));
      state = data_coin::orm::coin_transaction::state_t::stored;
    }
    else {
      switch ((data_coin::orm::coin_transaction::state_t)t1.state.get(st)){
        case data_coin::orm::coin_transaction::state_t::stored:{
          state = data_coin::orm::coin_transaction::state_t::stored;
          break;
        }
        case data_coin::orm::coin_transaction::state_t::validated: {
          if(
              data_coin::orm::coin_transaction::state_t::applied == state
              && this->applied_transactions_.end() == this->applied_transactions_.find(transaction_package.id())){
            state = data_coin::orm::coin_transaction::state_t::validated;
          }

          break;
        }
      }
    }
  }

  if(state == data_coin::orm::coin_transaction::state_t::stored){
    data_coin::orm::coin_transaction t1;
    t.execute(t1.insert(
        t1.id = id_str,
        t1.data = transaction_data,
        t1.state = (int)state));
    return;
  }

  //



}