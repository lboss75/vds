/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include <set>
#include "include/transaction_log.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "transaction_block_builder.h"
#include "transaction_log_hierarchy_dbo.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "vds_exceptions.h"
#include "logger.h"
#include "certificate_chain_dbo.h"
#include "include/transaction_state_calculator.h"
#include "transaction_log_vote_request_dbo.h"
#include "member_user_dbo.h"
#include "transaction_log_balance_dbo.h"

vds::const_data_buffer vds::transactions::transaction_log::save(
	const service_provider * sp,
	database_transaction & t,
	const const_data_buffer & block_data)
{
  transaction_block block(block_data);

  vds_assert(!block.exists(t));

  orm::transaction_log_record_dbo t1;
  t.execute(
    t1.insert(
      t1.id = block.id(),
      t1.data = block_data,
      t1.state = orm::transaction_log_record_dbo::state_t::validated,
      t1.new_member = false,
      t1.consensus = block.ancestors().empty(),
      t1.order_no = block.order_no(),
      t1.time_point = block.time_point()));

  block.walk_messages(
  [&t, log_id = block.id()](const root_user_transaction & message)->bool {
    orm::member_user_dbo t2;
    t.execute(
      t2.insert(
        t2.id = message.user_cert->subject(),
        t2.log_id = log_id));
    return true;
  },
  [&t, log_id = block.id()](const create_user_transaction & message)->bool {
    orm::member_user_dbo t2;
    t.execute(
      t2.insert(
        t2.id = message.user_cert->subject(),
        t2.log_id = log_id));
    return true;
  }
  );

  orm::transaction_log_hierarchy_dbo t2;
  for (const auto & ancestor : block.ancestors()) {
    t.execute(t2.insert(
      t2.id = ancestor,
      t2.follower_id = block.id()
    ));
  }

  process_block(sp, t, block_data);

  return block.id();
}

void vds::transactions::transaction_log::process_block(
  const service_provider* sp,
  database_transaction& t,
  const const_data_buffer& block_data) {
  transaction_block block(block_data);
  //Check user status
  orm::transaction_log_record_dbo t1;
  orm::member_user_dbo t5;
  auto st = t.get_reader(
    t5.select(t1.consensus)
    .inner_join(t1, t1.id == t5.log_id)
    .where(t5.id == block.write_cert_id()));
  if(!st.execute()) {
    throw std::runtime_error("Invalid data");
  }

  if(!t1.consensus.get(st)) {
    sp->get<logger>()->trace(
      ThisModule,
      "User %s is not in consensus. So ignore all actions from this user.",
      block.write_cert_id().c_str());

    return;
  }

  //Check ancestors
  std::set<const_data_buffer> remove_leaf;
  auto state = orm::transaction_log_record_dbo::state_t::leaf;
  for (const auto & ancestor : block.ancestors()) {
    auto st = t.get_reader(t1.select(t1.state,t1.order_no,t1.time_point).where(t1.id == ancestor));
    if (!st.execute()) {
      return;
    }
    else {
      if(t1.order_no.get(st) >= block.order_no() || t1.time_point.get(st) > block.time_point()) {
        state = orm::transaction_log_record_dbo::state_t::invalid;
        vds_assert(false);
      }
      else {
        switch (static_cast<orm::transaction_log_record_dbo::state_t>(t1.state.get(st))) {
        case orm::transaction_log_record_dbo::state_t::leaf: {
          remove_leaf.emplace(ancestor);
          break;
        }

        case orm::transaction_log_record_dbo::state_t::processed:
        case orm::transaction_log_record_dbo::state_t::validated:
          break;

        case orm::transaction_log_record_dbo::state_t::invalid:
          state = orm::transaction_log_record_dbo::state_t::invalid;
          break;

        default:
          throw std::runtime_error("Invalid program");
        }
      }
    }
  }

  try {
    auto datacoin_state = transaction_record_state::load(t, block);
    datacoin_state.save(t, block.write_cert_id(), block.id());
    //apply_block
  }
  catch(...) {
    state = orm::transaction_log_record_dbo::state_t::invalid;
  }

  if(check_consensus(t, block.id())) {
    t.execute(t1.update(t1.consensus = true).where(t1.id == block.id()));
  }


  update_consensus(sp, t, block_data);
  t.execute(t1.update(t1.state = state).where(t1.id == block.id()));

  if (orm::transaction_log_record_dbo::state_t::leaf == state) {
    for (const auto & p : remove_leaf) {
      t.execute(
        t1.update(
          t1.state = orm::transaction_log_record_dbo::state_t::processed)
        .where(t1.id == p));
    }
  }

  //process followers
  std::set<const_data_buffer> followers;
  orm::transaction_log_hierarchy_dbo t4;
  st = t.get_reader(t4.select(t4.follower_id).where(t4.id == block.id()));
  while (st.execute()) {
    const auto follower_id = t4.follower_id.get(st);
    if (follower_id) {
      followers.emplace(follower_id);
    }
  }

  for (const auto &p : followers) {

    st = t.get_reader(t1.select(t1.data).where(t1.id == p));
    if (!st.execute()) {
      throw std::runtime_error("Invalid data");
    }

    if(state == orm::transaction_log_record_dbo::state_t::leaf) {
      process_block(sp, t, t1.data.get(st));
    }
    else {
      invalid_block(sp, t, p);
    }
  }
}

void vds::transactions::transaction_log::update_consensus(
  const service_provider* sp,
  database_transaction& t,
  const const_data_buffer& block_data) {
  
  auto leaf_owner = transaction_block(block_data).write_cert_id();

  std::map<const_data_buffer, const_data_buffer> not_processed;
  std::map<const_data_buffer, const_data_buffer> processed;
  std::set<const_data_buffer> consensus_candidate;

  not_processed[transaction_block(block_data).id()] = block_data;

  while (!not_processed.empty()) {
    auto pbegin = not_processed.begin();
    auto data = pbegin->second;
    not_processed.erase(pbegin);

    transaction_block block(data);

    orm::transaction_log_record_dbo t1;
    for (const auto & ancestor : block.ancestors()) {
      if(processed.end() != processed.find(ancestor) || not_processed.end() != not_processed.find(ancestor)) {
        continue;
      }

      auto st = t.get_reader(t1.select(t1.state, t1.data).where(t1.id == ancestor));
      if (!st.execute()) {
        throw std::runtime_error("Invalid data");
      }

      auto ancestor_data = t1.data.get(st);

      orm::transaction_log_vote_request_dbo t2;
      t.execute(
        t2.update(t2.approved = true)
        .where(t2.id == ancestor && t2.owner == leaf_owner));

      if(check_consensus(t, ancestor)) {
        consensus_candidate.emplace(ancestor);
      }

      if(block.write_cert_id() != leaf_owner){
        not_processed[ancestor] = ancestor_data;
      }
    }
  }
  for(auto & candidate : consensus_candidate) {
    make_consensus(sp, t, candidate);
  }
}

void vds::transactions::transaction_log::invalid_block(
  const service_provider * sp,
  class database_transaction &t,
  const const_data_buffer & block_id) {

  orm::transaction_log_record_dbo t1;
  t.execute(t1.update(t1.state = orm::transaction_log_record_dbo::state_t::invalid).where(t1.id == block_id));

  std::set<const_data_buffer> followers;
  orm::transaction_log_hierarchy_dbo t4;
  auto st = t.get_reader(t4.select(t4.follower_id).where(t4.id == block_id));
  while (st.execute()) {
    const auto follower_id = t4.follower_id.get(st);
    if (follower_id) {
      followers.emplace(follower_id);
    }
  }

  for (const auto &p : followers) {

    st = t.get_reader(t1.select(t1.data).where(t1.id == p));
    if (!st.execute()) {
      throw std::runtime_error("Invalid data");
    }

    invalid_block(sp, t, p);
  }
}

void vds::transactions::transaction_log::invalid_become_consensus(const service_provider* sp,
  const database_transaction& t, const const_data_buffer& log_id) {
  throw std::runtime_error("Not implemented");

  //std::set<const_data_buffer> not_processed;
  //std::set<const_data_buffer> processed;

  //orm::transaction_log_record_dbo t1;
  //auto st = t.get_reader(t1.select(t1.order_no, t1.consensus).where(t1.state == orm::transaction_log_record_dbo::state_t::leaf));
  //while(st.execute()) {
  //  
  //}
}

void vds::transactions::transaction_log::make_consensus(const service_provider* sp, database_transaction& t,
  const const_data_buffer& start_log_id) {

  std::set<const_data_buffer> not_processed;
  std::set<const_data_buffer> processed;
  not_processed.emplace(start_log_id);

  while (!not_processed.empty()) {
    auto log_id = *not_processed.begin();
    not_processed.erase(not_processed.begin());
    processed.emplace(log_id);


    orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(t1.state, t1.data, t1.consensus).where(t1.id == log_id));
    if (!st.execute()) {
      throw std::runtime_error("Invalid data");
    }

    if (t1.consensus.get(st)) {
      continue;
    }

    auto state = t1.state.get(st);

    //check all ancestors in consensus
    auto all_ancestors_in_consensus = true;
    transaction_block block(t1.data.get(st));

    for (const auto & ancestor : block.ancestors()) {
      st = t.get_reader(t1.select(t1.consensus).where(t1.id == ancestor));
      if (!st.execute()) {
        throw std::runtime_error("Invalid data");
      }
      if (!t1.consensus.get(st)) {
        all_ancestors_in_consensus = false;
        break;
      }
    }

    if(!all_ancestors_in_consensus) {
      continue;
    }

    t.execute(
      t1.update(
        t1.consensus = true)
      .where(t1.id == log_id));

    switch (state) {
    case orm::transaction_log_record_dbo::state_t::invalid:
      invalid_become_consensus(sp, t, log_id);
      break;

    case orm::transaction_log_record_dbo::state_t::processed: {
      std::set<const_data_buffer> followers;
      orm::transaction_log_hierarchy_dbo t3;
      st = t.get_reader(t3.select(t3.follower_id).where(t3.id == log_id));
      while(st.execute()) {
        auto follower_id = t3.follower_id.get(st);
        if(not_processed.end() == not_processed.find(follower_id)
          && processed.end() == processed.find(follower_id)) {
          followers.emplace(follower_id);
        }
      }

      for(const auto & follower_id : followers) {
        if (check_consensus(t, follower_id)) {
          not_processed.emplace(follower_id);
        }
      }

      break;
    }

    case orm::transaction_log_record_dbo::state_t::leaf: {
      break;
    }

    default:
      throw std::runtime_error("Invalid program");
    }
  }
}

bool vds::transactions::transaction_log::check_consensus(
  database_read_transaction& t,
  const const_data_buffer & log_id) {

  orm::transaction_log_vote_request_dbo t2;

  db_value<int> appoved_count;
  auto st = t.get_reader(
    t2.select(db_count(t2.owner).as(appoved_count))
    .where(t2.id == log_id && t2.approved == true));
  if (!st.execute()) {
    throw std::runtime_error("Invalid data");
  }
  auto ac = appoved_count.get(st);

  db_value<int> total_count;
  st = t.get_reader(
    t2.select(db_count(t2.owner).as(total_count))
    .where(t2.id == log_id));
  if (!st.execute()) {
    throw std::runtime_error("Invalid data");
  }
  auto tc = total_count.get(st);

  return (ac > tc / 2);
}
