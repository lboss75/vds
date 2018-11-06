///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//
//#include "private/stdafx.h"
//#include "transaction_state_calculator.h"
//#include "database.h"
//#include "transaction_log_record_dbo.h"
//#include "encoding.h"
//#include "transaction_block_builder.h"
//#include "vds_debug.h"
//#include "transaction_block.h"
//#include "transaction_log_vote_request_dbo.h"
//#include "transaction_log_hierarchy_dbo.h"
//
//
//vds::transactions::transaction_record_state
//vds::transactions::transaction_state_calculator::calculate(
//    database_read_transaction &t,
//    const std::set<vds::const_data_buffer> & ancestors,
//    const std::chrono::system_clock::time_point & time_point,
//    uint64_t max_order_no) {
//
//  transaction_state_calculator calculator;
//  for(const auto & ancestor : ancestors){
//    calculator.add_ancestor(t, ancestor, time_point, max_order_no);
//  }
//
//  return calculator.process(t);
//}
//
//
//
//vds::transactions::transaction_record_state
//vds::transactions::transaction_state_calculator::process(vds::database_read_transaction &t) {
//  while(1 < this->not_processed_.size()
//        || (1 == this->not_processed_.size() && 1 < this->not_processed_.begin()->second.size())){
//    auto p = this->not_processed_.rbegin();
//    auto pnode = p->second.begin();
//
//    const auto order_no = p->first;
//    const auto node_id = *pnode;
//    p->second.erase(pnode);
//    if(p->second.empty()){
//      this->not_processed_.erase(order_no);
//    }
//
//    this->resolve(t, order_no, node_id);
//  }
//
//  if(this->not_processed_.empty()){
//    throw std::runtime_error("Invalid program");
//  }
//  auto log_id = *this->not_processed_.begin()->second.begin();
//  auto result = this->load_state(t, log_id);
//  
//  for (const auto & porder : this->items_) {
//    for (const auto & pitem : porder.second) {
//      transaction_block block(pitem.second);
//      result.apply(block);
//    }
//  }
//
//  return result;
//}
//
//vds::transactions::transaction_record_state &
//vds::transactions::transaction_state_calculator::load_state(
//  vds::database_read_transaction &t,
//  const const_data_buffer & log_id) {
//
//  auto p = this->balance_cache_.find(log_id);
//  if(this->balance_cache_.end() != p) {
//    return p->second;
//  }
//
//  this->balance_cache_[log_id] = transactions::transaction_record_state::load(t, log_id);
//  return this->balance_cache_[log_id];
//
//  const auto state_data = t1.state_data.get(st);
//  binary_deserializer s(state_data);
//  transaction_record_state result(s);
//  result.reset_consensus();
//
//  for (const auto & porder : this->items_) {
//    for (const auto & pitem : porder.second) {
//      transaction_block block(pitem.second);
//      result.apply(block);
//    }
//  }
//
//}
//
//void vds::transactions::transaction_state_calculator::resolve(
//    vds::database_read_transaction &t,
//    const uint64_t parent_order_no,
//    const vds::const_data_buffer &node_id) {
//
//  orm::transaction_log_record_dbo t1;
//  auto st = t.get_reader(
//      t1.select(t1.data)
//          .where(t1.id == node_id));
//  if(!st.execute()){
//    throw std::runtime_error("Invalid data");
//  }
//
//  const auto block_data = t1.data.get(st);
//
//  transaction_block block(block_data);
//
//  vds_assert(block.order_no() == parent_order_no);
//
//  this->items_[block.order_no()][node_id] = block_data;
//
//  for(const auto & ancestor : block.ancestors()){
//    this->add_ancestor(t, ancestor, block.time_point(), block.order_no());
//  }
//}
//
//vds::const_data_buffer vds::transactions::transaction_state_calculator::looking_leaf(
//  database_read_transaction& t,
//  const const_data_buffer& log_id) {
//  std::set<const_data_buffer/*log_id*/> not_processed;
//  std::set<const_data_buffer/*log_id*/> processed;
//
//  not_processed.emplace(log_id);
//  while(!not_processed.empty()){
//    auto p = *not_processed.begin();
//    not_processed.erase(p);
//    processed.emplace(p);
//
//    orm::transaction_log_hierarchy_dbo t1;
//    auto st = t.get_reader(t1.select(t1.follower_id).where(t1.id == p));
//    bool is_leaf = true;
//    while(st.execute()) {
//      is_leaf = false;
//
//      auto follower = t1.follower_id.get(st);
//      if(not_processed.end() == not_processed.find(follower)
//        && processed.end() == processed.find(follower)) {
//        not_processed.emplace(follower);
//      }
//    }
//
//    if(is_leaf) {
//      return p;
//    }
//  }
//
//  throw std::runtime_error("Leaf not found");
//}
//
//vds::transactions::transaction_record_state vds::transactions::transaction_state_calculator::calculate_state(
//  database_read_transaction& t,
//  const const_data_buffer& log_id) {
//
//  auto leaf = looking_leaf(t, log_id);
//
//}
