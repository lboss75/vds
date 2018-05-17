/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "include/transaction_record_state.h"
#include "encoding.h"
#include "private/transaction_source_not_found_error.h"
#include "private/transaction_lack_of_funds.h"


void vds::transactions::transaction_record_state::apply(
  const std::string & souce_account,
  const const_data_buffer& transaction_id,
  const const_data_buffer& messages) {

  transaction_log::walk_messages(
    messages,
    [this, souce_account, transaction_id](payment_transaction & t)->bool {
    auto p = this->account_state_.find(souce_account);
    if (this->account_state_.end() == p) {
      return false;
    }

    auto p1 = p->second.find(t.source_transaction());
    if (p->second.end() == p1) {
      throw transaction_source_not_found_error(t.source_transaction(), transaction_id);
    }

    if (p1->second < t.value()) {
      throw transaction_lack_of_funds(t.source_transaction(), transaction_id);
    }

    p1->second -= t.value();
    this->account_state_[t.target_user()][transaction_id] += t.value();
    return true;
  });
}