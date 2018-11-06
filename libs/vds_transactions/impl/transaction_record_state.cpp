/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "transaction_record_state.h"
#include "transaction_source_not_found_error.h"
#include "transaction_lack_of_funds.h"
#include "transaction_block.h"
#include "transaction_log_vote_request_dbo.h"
#include "transaction_log_record_dbo.h"
#include "transaction_log_hierarchy_dbo.h"

vds::transactions::transaction_record_state vds::transactions::transaction_record_state::load(
  database_read_transaction& t, const const_data_buffer& log_id) {

  transaction_state_calculator calculator;
  calculator.add_ancestor(t, log_id);
  return calculator.load(t);
}

vds::transactions::transaction_record_state vds::transactions::transaction_record_state::load(
  database_read_transaction & t,
  const std::set<vds::const_data_buffer>& ancestors)
{
  transaction_state_calculator calculator;
  for (auto & p : ancestors) {
    calculator.add_ancestor(t, p);
  }

  return calculator.load(t);
}

vds::transactions::transaction_record_state vds::transactions::transaction_record_state::load(
  database_read_transaction& t, const transaction_block & block) {
  if(block.ancestors().empty()) {
    transaction_record_state result;
    result.apply(block);
    return result;
  }
  else {
    auto result = load(t, block.ancestors());
    result.apply(block);
    return result;
  }
}

void vds::transactions::transaction_record_state::save(
  database_transaction& t,
  const std::string & owner,
  const const_data_buffer& log_id) const {
  for (const auto & account : this->account_state_) {
    for (const auto & account_state : account.second.balance_) {
      orm::transaction_log_vote_request_dbo t1;
      t.execute(t1.insert(
        t1.id = log_id,
        t1.owner = account.first,
        t1.source = account_state.first,
        t1.balance = account_state.second,
        t1.is_appoved = (account.first == owner)));
    }
  }
}

void vds::transactions::transaction_record_state::apply(
  const transaction_block & block) {

  block.walk_messages(
    [this, &block](const payment_transaction & t)->bool {
    auto p = this->account_state_.find(block.write_cert_id());
    if (this->account_state_.end() == p) {
      throw transaction_source_not_found_error(t.source_transaction, block.id());
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
  },
    [this, &block](const root_user_transaction & t)->bool {
#undef  max
    this->account_state_[block.write_cert_id()].balance_[block.id()] = std::numeric_limits<int64_t>::max();
    return true;
  });
}

void vds::transactions::transaction_record_state::rollback(
  const transaction_block & block) {

  block.walk_messages(
    [this, &block](const payment_transaction & t)->bool {
    this->account_state_[block.write_cert_id()].balance_[t.source_transaction] += t.value;
    this->account_state_[t.target_user].balance_[block.id()] -= t.value;
    return true;
  },
    [this, &block](const root_user_transaction & t)->bool {
    throw std::runtime_error("Invalid operation");
  });
}

vds::transactions::transaction_record_state::transaction_state_calculator::transaction_state_calculator()
: not_included_(0) {
}

void vds::transactions::transaction_record_state::transaction_state_calculator::add_ancestor(
  vds::database_read_transaction& t, const const_data_buffer& ancestor_id) {
  orm::transaction_log_record_dbo t2;
  auto st = t.get_reader(
    t2.select(
      t2.order_no)
    .where(t2.id == ancestor_id));
  if (!st.execute()) {
    throw std::runtime_error("Invalid data");
  }

  this->set_state(t2.order_no.get(st), ancestor_id, log_state_t::include);
}

vds::transactions::transaction_record_state vds::transactions::transaction_record_state::transaction_state_calculator::load_init_state(
  vds::database_read_transaction& t) {

  vds_assert(!this->items_.empty());
  vds_assert(!this->items_.rbegin()->second.empty());

  //looking for leaf
  std::set<const_data_buffer/*log_id*/> not_processed;
  std::set<const_data_buffer/*log_id*/> processed;

  not_processed.emplace(this->items_.rbegin()->second.begin()->first);

  transaction_record_state result;
  bool state_loaded = false;

  while (!not_processed.empty()) {
    auto p = *not_processed.begin();
    not_processed.erase(p);
    processed.emplace(p);

    orm::transaction_log_record_dbo t2;
    auto st = t.get_reader(t2.select(t2.state, t2.order_no).where(t2.id == p));

    if (!st.execute()) {
      throw std::runtime_error("Invalid data");
    }

    if (orm::transaction_log_record_dbo::have_state(t2.state.get(st))) {
      this->set_state(t2.order_no.get(st), p, log_state_t::exclude);
      orm::transaction_log_vote_request_dbo t3;
      st = t.get_reader(t3.select(t3.owner, t3.source, t3.balance).where(t3.id == p));
      while (st.execute()) {
        result.account_state_[t3.owner.get(st)].balance_[t3.source.get(st)] = t3.balance.get(st);
      }

      state_loaded = true;
      break;
    }

    orm::transaction_log_hierarchy_dbo t1;
    st = t.get_reader(
      t1.select(t1.follower_id)
      .where(t1.id == p));

    while (st.execute()) {
      auto follower = t1.follower_id.get(st);
      if (not_processed.end() == not_processed.find(follower)
        && processed.end() == processed.find(follower)) {
        not_processed.emplace(follower);
      }
    }
  }

  vds_assert(state_loaded);

  return result;
}

vds::transactions::transaction_record_state vds::transactions::transaction_record_state::transaction_state_calculator::load(
  vds::database_read_transaction& t) {

  auto result = this->load_init_state(t);

  auto porder_no = this->items_.rbegin();
  while (this->items_.rend() != porder_no && 0 != this->not_included_) {
    for (auto & p : porder_no->second) {
      orm::transaction_log_record_dbo t1;
      auto st = t.get_reader(
        t1.select(t1.data)
        .where(t1.id == p.first));

      if (!st.execute()) {
        throw std::runtime_error("Invalid data");
      }

      transaction_block block(t1.data.get(st));
      switch (p.second.state_) {
      case log_state_t::included: {
        break;
      }

      case log_state_t::exclude: {
        this->not_included_--;
        result.rollback(block);
        break;
      }

      case log_state_t::include: {
        this->not_included_--;
        result.apply(block);
        break;
      }
      default:
        throw std::runtime_error("Invalid data");
      }

      for (auto & ancestor_id : block.ancestors()) {
        st = t.get_reader(
          t1.select(t1.order_no)
          .where(t1.id == ancestor_id));

        if (!st.execute()) {
          throw std::runtime_error("Invalid data");
        }

        vds_assert(t1.order_no.get(st) < porder_no->first);
        this->set_state(t1.order_no.get(st), ancestor_id, p.second.state_);
      }
    }
    
    this->items_.erase(porder_no->first);
    porder_no = this->items_.rbegin();
  }

  vds_assert(0 == this->not_included_);

  return result;
}

void vds::transactions::transaction_record_state::transaction_state_calculator::set_state(uint64_t order_no,
  const vds::const_data_buffer& log_id, log_state_t state) {

  auto pitems = this->items_.find(order_no);
  if (this->items_.end() != pitems) {
    auto pstate = pitems->second.find(log_id);
    if (pitems->second.end() != pstate) {
      if ((state == log_state_t::included || state == log_state_t::include)
        && pstate->second.state_ != log_state_t::included) {
        
        if (state != log_state_t::included) {
          this->not_included_++;
        }

        if(pstate->second.state_ != log_state_t::included) {
          this->not_included_--;
        }
        /*  exist\new | include | exclude|included
         *  include   | include |included|included
         *  exclude   | include | exclude|included
         *  included  | included|included|included
         */
        pstate->second.state_ = state;
      }
      else if(state == log_state_t::exclude && pstate->second.state_ == log_state_t::include){
        this->not_included_--;
        pstate->second.state_ = log_state_t::included;        
      }
    }
    else {
      if (state != log_state_t::included) {
        this->not_included_++;
      }
      pitems->second[log_id].state_ = state;
    }
  }
  else {
    if (state != log_state_t::included) {
      this->not_included_++;
    }
    this->items_[order_no][log_id].state_ = state;
  }
}

vds::transactions::transaction_record_state::transaction_state_calculator::log_state_t vds::transactions::
transaction_record_state::transaction_state_calculator::get_state(
  uint64_t order_no,
  const vds::const_data_buffer& log_id) const {

  auto pitems = this->items_.find(order_no);
  if (this->items_.end() != pitems) {
    auto pstate = pitems->second.find(log_id);
    if (pitems->second.end() != pstate) {
      return pstate->second.state_;
    }
    else {
      return log_state_t::none;
    }
  }
  else {
    return log_state_t::none;
  }
}

