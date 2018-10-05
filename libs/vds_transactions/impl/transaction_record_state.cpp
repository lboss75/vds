/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "transaction_record_state.h"
#include "transaction_source_not_found_error.h"
#include "transaction_lack_of_funds.h"
#include "transaction_block.h"

void vds::transactions::transaction_record_state::apply(
  const transaction_block & block) {

  block.walk_messages(
    [this, &block](const payment_transaction & t)->bool {
    auto p = this->account_state_.find(block.write_cert_id());
    if (this->account_state_.end() == p) {
      return false;
    }

    auto p1 = p->second.balance_.find(t.source_transaction);
    if (p->second.balance_.end() == p1) {
      throw transaction_source_not_found_error(t.source_transaction, block.id());
    }

    if (p1->second < t.value) {
      throw transaction_lack_of_funds(t.source_transaction, block.id());
    }

    p1->second -= t.value;
    this->account_state_[t.target_user].balance_[block.id()] += t.value;
    return true;
  });
}
