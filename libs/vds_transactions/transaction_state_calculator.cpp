/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include "private/transaction_state_calculator.h"
#include "database.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "include/transaction_block_builder.h"
#include "vds_debug.h"
#include "transaction_block.h"

vds::transactions::transaction_record_state
vds::transactions::transaction_state_calculator::calculate(
    database_transaction &t,
    const std::set<vds::const_data_buffer> & ancestors,
    uint64_t max_order_no) {

  transaction_state_calculator calculator;
  for(const auto & ancestor : ancestors){
    calculator.add_ancestor(t, ancestor, max_order_no);
  }

  return calculator.process(t);
}

void vds::transactions::transaction_state_calculator::add_ancestor(
    vds::database_transaction &t,
    const vds::const_data_buffer &ancestor_id,
    uint64_t max_order_no) {

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(
              t1.id,
              t1.order_no)
          .where(t1.id == base64::from_bytes(ancestor_id)));
  if(!st.execute()){
    throw std::runtime_error("Invalid data");
  }

  const auto id = base64::to_bytes(t1.id.get(st));
  const auto order_no = t1.order_no.get(st);
  vds_assert(order_no < max_order_no);

  auto pitems = this->items_.find(order_no);
  if(this->items_.end() != pitems){
    if(pitems->second.end() != pitems->second.find(id)){
      return;//Already processed
    }
  }

  if(this->not_processed_[order_no].end() == this->not_processed_[order_no].find(id)) {
    this->not_processed_[order_no].emplace(id);
  }
}

vds::transactions::transaction_record_state
vds::transactions::transaction_state_calculator::process(vds::database_transaction &t) {
  while(1 < this->not_processed_.size()
        || (1 == this->not_processed_.size() && 1 < this->not_processed_.begin()->second.size())){
    auto p = this->not_processed_.rbegin();
    auto pnode = p->second.begin();

    const auto order_no = p->first;
    const auto node_id = *pnode;
    p->second.erase(pnode);
    if(p->second.empty()){
      this->not_processed_.erase(order_no);
    }

    this->resolve(t, order_no, node_id);
  }

  if(this->not_processed_.size() == 0){
    throw std::runtime_error("Invalid program");
  }

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.state_data)
          .where(t1.id == base64::from_bytes(*this->not_processed_.begin()->second.begin())));
  if(!st.execute()){
    throw std::runtime_error("Invalid data");
  }

  const auto state_data = t1.state_data.get(st);
  binary_deserializer s(state_data);
  transaction_record_state result(s);

  for(const auto & porder : this->items_){
    for(const auto & pitem : porder.second){
      result.apply(pitem.second);
    }
  }

  return result;
}

void vds::transactions::transaction_state_calculator::resolve(
    vds::database_transaction &t,
    const uint64_t parent_order_no,
    const vds::const_data_buffer &node_id) {

  orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.data)
          .where(t1.id == base64::from_bytes(node_id)));
  if(!st.execute()){
    throw std::runtime_error("Invalid data");
  }

  const auto block_data = t1.data.get(st);

  transaction_block block(block_data);

  vds_assert(block.order_no() == parent_order_no);

  this->items_[order_no][node_id] = block_messages;

  for(const auto & ancestor : ancestors){
    this->add_ancestor(t, ancestor, order_no);
  }
}
