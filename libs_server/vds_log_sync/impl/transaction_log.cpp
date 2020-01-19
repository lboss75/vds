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
#include "channel_message_dbo.h"
#include "include/transaction_state_calculator.h"
#include "transaction_log_vote_request_dbo.h"
#include "member_user_dbo.h"
#include "transaction_log_balance_dbo.h"
#include "database.h"
#include "datacoin_balance_dbo.h"
#include "node_info_dbo.h"
#include "wallet_dbo.h"
#include "transaction_block.h"
#include "dht_network_client.h"
#include "../private/dht_network_client_p.h"
#include <chunk_tmp_data_dbo.h>
#include <chunk_replica_data_dbo.h>
#include <sync_replica_map_dbo.h>

vds::expected<vds::const_data_buffer> vds::transactions::transaction_log::save(
	const service_provider * sp,
	database_transaction & t,
	const const_data_buffer & block_data)
{
  GET_EXPECTED(block, transaction_block::create(block_data));

  GET_EXPECTED(block_exists, block.exists(t));
  vds_assert(!block_exists);

  orm::transaction_log_record_dbo t1;
  CHECK_EXPECTED(
  t.execute(
    t1.insert(
      t1.id = block.id(),
      t1.data = block_data,
      t1.state = orm::transaction_log_record_dbo::state_t::validated,
      t1.consensus = false,
      t1.order_no = block.order_no(),
      t1.time_point = block.time_point())));

  orm::transaction_log_hierarchy_dbo t2;
  for (const auto & ancestor : block.ancestors()) {
    CHECK_EXPECTED(t.execute(t2.insert(
      t2.id = ancestor,
      t2.follower_id = block.id()
    )));
  }

  CHECK_EXPECTED(
    process_block_with_followers(
      sp,
      t,
      block.id(),
      block_data, orm::transaction_log_record_dbo::state_t::validated,
      block.ancestors().empty()));

  return block.id();
}

vds::expected<void> vds::transactions::transaction_log::process_block_with_followers(
  const service_provider * sp,
  database_transaction & t,
  const const_data_buffer & block_id,
  const const_data_buffer & block_data,
  orm::transaction_log_record_dbo::state_t state,
  bool in_consensus)
{
  std::map<const_data_buffer, std::tuple<const_data_buffer, orm::transaction_log_record_dbo::state_t, bool>> not_processed;
  std::set<const_data_buffer> processed;

  not_processed[block_id] = std::make_tuple(block_data, state, in_consensus);

  while (!not_processed.empty()) {
    auto pbegin = not_processed.begin();
    const auto data = pbegin->second;
    not_processed.erase(pbegin);

    GET_EXPECTED(current_block, transaction_block::create(std::get<0>(data)));
    processed.emplace(current_block.id());

    GET_EXPECTED(result, process_block(sp, t, current_block, std::get<2>(data)));
    if (vds::orm::transaction_log_record_dbo::state_t::validated == result) {
      continue;
    }

    GET_EXPECTED(update_consensus_result, update_consensus(sp, t, current_block, std::get<0>(data), result, std::get<2>(data)));
    if (!update_consensus_result) {
      return expected<void>();
    }

    //process followers
    std::set<const_data_buffer> followers;
    orm::transaction_log_hierarchy_dbo t4;
    GET_EXPECTED(st, t.get_reader(t4.select(t4.follower_id).where(t4.id == current_block.id())));
    WHILE_EXPECTED(st.execute()) {
      const auto follower_id = t4.follower_id.get(st);
      if (follower_id) {
        followers.emplace(follower_id);
      }
    }
    WHILE_EXPECTED_END()

    for (const auto &p : followers) {
      if (processed.end() != processed.find(p) || not_processed.end() != not_processed.find(p)) {
        continue;
      }

      orm::transaction_log_record_dbo t1;
      GET_EXPECTED_VALUE(st, t.get_reader(t1.select(t1.data, t1.state, t1.consensus).where(t1.id == p)));
      GET_EXPECTED(st_execute_result, st.execute());
      if (!st_execute_result) {
        return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      if (result == orm::transaction_log_record_dbo::state_t::leaf) {
        not_processed[p] = std::make_tuple(t1.data.get(st), t1.state.get(st), t1.consensus.get(st));
      }
      else {
        CHECK_EXPECTED(invalid_block(sp, t, p, true));
      }
    }
  }

  return expected<void>();
}

vds::expected<vds::orm::transaction_log_record_dbo::state_t> vds::transactions::transaction_log::process_block(
  const service_provider* sp,
  database_transaction& t,
  const transaction_block & block,
  bool in_consensus) {

  orm::transaction_log_record_dbo t1;
  orm::transaction_log_vote_request_dbo t2;

  //Check ancestors
  std::set<const_data_buffer> remove_leaf;
  auto state = orm::transaction_log_record_dbo::state_t::leaf;
  std::set<const_data_buffer> vote_requests;
  for (const auto & ancestor : block.ancestors()) {
    GET_EXPECTED(st, t.get_reader(t2.select(t2.owner).where(t2.id == ancestor)));
    WHILE_EXPECTED(st.execute())
    {
      const auto owner = t2.owner.get(st);
      if (vote_requests.end() == vote_requests.find(owner)) {
        vote_requests.emplace(owner);
      }
    }
    WHILE_EXPECTED_END();
    
    GET_EXPECTED_VALUE(st, t.get_reader(t1.select(t1.state,t1.order_no,t1.time_point).where(t1.id == ancestor)));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute){
      return vds::orm::transaction_log_record_dbo::state_t::validated;
    }
    else {
      if(safe_cast<uint64_t>(t1.order_no.get(st)) >= block.order_no() || t1.time_point.get(st) > block.time_point()) {
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
          break;

        case orm::transaction_log_record_dbo::state_t::validated: {
          sp->get<logger>()->trace(
            ThisModule,
            "Ancestor %s is not processed. So stop processing this block.",
            base64::from_bytes(ancestor).c_str());
          return vds::orm::transaction_log_record_dbo::state_t::validated;
        }

        case orm::transaction_log_record_dbo::state_t::invalid:
          state = orm::transaction_log_record_dbo::state_t::invalid;
          break;

        default:
          return vds::make_unexpected<std::runtime_error>("Invalid program");
        }
      }
    }
  }

  GET_EXPECTED(st, t.get_reader(t2.select(t2.owner).where(t2.id == block.id())));
  WHILE_EXPECTED(st.execute())
  {
    const auto owner = t2.owner.get(st);
    auto p = vote_requests.find(owner);
    if (vote_requests.end() != p) {
      vote_requests.erase(p);
    }
  }
  WHILE_EXPECTED_END();

  for (const auto owner : vote_requests) {
    CHECK_EXPECTED(
      t.execute(
        t2.insert(
          t2.id = block.id(),
          t2.owner = owner,
          t2.approved = false,
          t2.new_member = false)));
  }
  
  if (orm::transaction_log_record_dbo::state_t::leaf == state
    || orm::transaction_log_record_dbo::state_t::processed == state) {
    GET_EXPECTED(is_processed, process_records(sp, t, block));
    if (!is_processed) {
      state = orm::transaction_log_record_dbo::state_t::invalid;
    }
  }

  CHECK_EXPECTED(t.execute(t1.update(t1.state = state).where(t1.id == block.id())));

  if (orm::transaction_log_record_dbo::state_t::leaf == state) {
    for (const auto & p : remove_leaf) {
      CHECK_EXPECTED(t.execute(
        t1.update(
          t1.state = orm::transaction_log_record_dbo::state_t::processed)
        .where(t1.id == p)));
    }
  }

  return state;
}

vds::expected<void> vds::transactions::transaction_log::process_followers(const service_provider * sp, database_transaction & t)
{
  std::map<const_data_buffer, std::tuple<const_data_buffer, orm::transaction_log_record_dbo::state_t, bool>> not_processed;
  std::set<const_data_buffer> processed;

  orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.id, t1.data, t1.consensus).where(t1.state == orm::transaction_log_record_dbo::state_t::leaf)));
  WHILE_EXPECTED(st.execute()) {
    not_processed[t1.id.get(st)] = std::make_tuple(t1.data.get(st), orm::transaction_log_record_dbo::state_t::leaf, t1.consensus.get(st));
  }
  WHILE_EXPECTED_END()


  while (!not_processed.empty()) {
    auto pbegin = not_processed.begin();
    const auto data = pbegin->second;
    not_processed.erase(pbegin);

    GET_EXPECTED(current_block, transaction_block::create(std::get<0>(data)));
    processed.emplace(current_block.id());

    GET_EXPECTED(result, process_block(sp, t, current_block, std::get<2>(data)));

    GET_EXPECTED(update_consensus_result, update_consensus(sp, t, current_block, std::get<0>(data), std::get<1>(data), std::get<2>(data)));
    if (!update_consensus_result) {
      return expected<void>();
    }

    //process followers
    std::set<const_data_buffer> followers;
    orm::transaction_log_hierarchy_dbo t4;
    GET_EXPECTED(st, t.get_reader(t4.select(t4.follower_id).where(t4.id == current_block.id())));
    WHILE_EXPECTED(st.execute()) {
      const auto follower_id = t4.follower_id.get(st);
      if (follower_id) {
        followers.emplace(follower_id);
      }
    }
    WHILE_EXPECTED_END()

    for (const auto &p : followers) {
      if (processed.end() != processed.find(p) || not_processed.end() != not_processed.find(p)) {
        continue;
      }

      orm::transaction_log_record_dbo t1;
      GET_EXPECTED_VALUE(st, t.get_reader(t1.select(t1.data, t1.state, t1.consensus).where(t1.id == p)));
      GET_EXPECTED(st_execute_result, st.execute());
      if (!st_execute_result) {
        return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      if (result == orm::transaction_log_record_dbo::state_t::leaf) {
        not_processed[p] = std::make_tuple(t1.data.get(st), t1.state.get(st), t1.consensus.get(st));
      }
      else {
        CHECK_EXPECTED(invalid_block(sp, t, p, true));
      }
    }
  }

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::update_consensus(
  const service_provider* sp,
  database_transaction& t,
  const transaction_block & block,
  const const_data_buffer& block_data,
  orm::transaction_log_record_dbo::state_t state,
  bool in_consensus) {

  std::map<const_data_buffer, std::tuple<const_data_buffer, orm::transaction_log_record_dbo::state_t, bool>> not_processed;
  std::set<const_data_buffer> processed;
  std::list<std::tuple<const_data_buffer, const_data_buffer, orm::transaction_log_record_dbo::state_t, bool>> consensus_candidate;
  bool have_invalid_consensus = false;
  uint64_t min_order;
  orm::transaction_log_record_dbo t1;

  not_processed[block.id()] = std::make_tuple(block_data, state, in_consensus);

  while (!not_processed.empty()) {
    auto pbegin = not_processed.begin();
    const auto data = pbegin->second;
    not_processed.erase(pbegin);

    GET_EXPECTED(current_block, transaction_block::create(std::get<0>(data)));
    processed.emplace(current_block.id());

    bool is_new;
    orm::transaction_log_vote_request_dbo t2;
    GET_EXPECTED(st, t.get_reader(t2.select(t2.approved, t2.new_member).where(t2.id == current_block.id() && t2.owner == block.write_public_key_id())));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute) {
      if (std::get<2>(data)) {
        CHECK_EXPECTED(t.execute(
          t2.insert(
            t2.approved = true,
            t2.id = current_block.id(),
            t2.owner = block.write_public_key_id(),
            t2.new_member = true)));
        is_new = true;
      }
      else {
        return vds::make_unexpected<std::runtime_error>("Invalid data");
      }
    }
    else {
      if (t2.approved.get(st)) {
        continue;
      }
      is_new = t2.new_member.get(st);

      CHECK_EXPECTED(t.execute(
        t2.update(t2.approved = true)
        .where(t2.id == current_block.id() && t2.owner == block.write_public_key_id())));
    }
    GET_EXPECTED(check_consensus_result, check_consensus(t, current_block.id()));
    if (check_consensus_result && !std::get<2>(data)) {
      if (consensus_candidate.empty() || min_order > current_block.order_no()) {
        min_order = current_block.order_no();
      }

      consensus_candidate.push_front(std::make_tuple(current_block.id(), std::get<0>(data), std::get<1>(data), std::get<2>(data)));

      if (!have_invalid_consensus && orm::transaction_log_record_dbo::state_t::invalid == std::get<1>(data)) {
        have_invalid_consensus = true;
      }
    }
    
    if ((current_block.write_public_key_id() != block.write_public_key_id() || current_block.id() == block.id()) && !is_new) {
      for (const auto & ancestor : current_block.ancestors()) {
        if (processed.end() != processed.find(ancestor) || not_processed.end() != not_processed.find(ancestor)) {
          continue;
        }

        GET_EXPECTED(st, t.get_reader(t1.select(t1.state, t1.data, t1.consensus).where(t1.id == ancestor)));
        GET_EXPECTED(st_execute, st.execute());
        if (!st_execute) {
          return vds::make_unexpected<std::runtime_error>("Invalid data");
        }

        not_processed[ancestor] = std::make_tuple(t1.data.get(st), t1.state.get(st), t1.consensus.get(st));
      }
    }
  }
  processed.clear();

  if (consensus_candidate.empty()) {
    return true;
  }

  for (const auto & p : consensus_candidate) {
    orm::transaction_log_record_dbo t1;
    CHECK_EXPECTED(t.execute(
      t1
      .update(t1.consensus = true)
      .where(t1.id == std::get<0>(p))));

    if (orm::transaction_log_record_dbo::state_t::processed == std::get<2>(p)
        || orm::transaction_log_record_dbo::state_t::leaf == std::get<2>(p)) {
      GET_EXPECTED(block, transaction_block::create(std::get<1>(p)));
      CHECK_EXPECTED(consensus_records(sp, t, block));
    }
    else if (!have_invalid_consensus) {
      return vds::make_unexpected<std::runtime_error>("Invalid program");
    }
  }

  if (!have_invalid_consensus) {
    return true;
  }

  CHECK_EXPECTED(rollback_all(sp, t, min_order));

  for (const auto & p : consensus_candidate) {
    GET_EXPECTED(block, transaction_block::create(std::get<1>(p)));
    if (orm::transaction_log_record_dbo::state_t::invalid == std::get<2>(p)) {
      CHECK_EXPECTED(invalid_block(sp, t, std::get<0>(p), false));
      GET_EXPECTED(state, process_block(sp, t, block, std::get<3>(p)));
      if (orm::transaction_log_record_dbo::state_t::leaf != state) {
        return vds::make_unexpected<std::runtime_error>("Invalid program");
      }

      CHECK_EXPECTED(consensus_records(sp, t, block));
    }
  }

  std::set<const_data_buffer> followers;
  orm::transaction_log_hierarchy_dbo t4;
  GET_EXPECTED(st, t.get_reader(
    t4
    .select(t4.follower_id)
    .inner_join(t1, t1.id == t4.id)
    .where(
      t1.state == orm::transaction_log_record_dbo::state_t::leaf)));
  WHILE_EXPECTED(st.execute()) {
    const auto follower_id = t4.follower_id.get(st);
    if (follower_id && followers.end() == followers.find(follower_id)) {
      followers.emplace(follower_id);
    }
  }
  WHILE_EXPECTED_END()

  for (const auto & p : followers) {
    GET_EXPECTED(st, t.get_reader(t1.select(t1.state, t1.data, t1.consensus).where(t1.id == p)));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute) {
      return vds::make_unexpected<std::runtime_error>("Invalid data");
    }
    if (orm::transaction_log_record_dbo::state_t::validated != t1.state.get(st)) {
      continue;
    }
    CHECK_EXPECTED(process_block_with_followers(sp, t, p, t1.data.get(st), t1.state.get(st), t1.consensus.get(st)));
  }

  return false;
}

vds::expected<void> vds::transactions::transaction_log::rollback_all(const service_provider * sp, database_transaction & t, uint64_t min_order)
{
  std::map<const_data_buffer, const_data_buffer> not_processed;
  std::set<const_data_buffer> processed;

  orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st,
    t.get_reader(
      t1.select(t1.id, t1.data)
      .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf && t1.order_no >= safe_cast<int64_t>(min_order) && t1.consensus == false)));

  WHILE_EXPECTED(st.execute()) {
    not_processed[t1.id.get(st)] = t1.data.get(st);
  }
  WHILE_EXPECTED_END()

  while (!not_processed.empty()) {
    auto pbegin = not_processed.begin();
    GET_EXPECTED(current_block, transaction_block::create(pbegin->second));
    processed.emplace(current_block.id());
    not_processed.erase(pbegin);

    CHECK_EXPECTED(undo_records(sp, t, current_block));

    CHECK_EXPECTED(t.execute(
        t1.update(t1.state = orm::transaction_log_record_dbo::state_t::validated)
        .where(t1.id == current_block.id())));

    for(const auto & ancestor : current_block.ancestors()){
      if (processed.end() != processed.find(ancestor) || not_processed.end() != not_processed.find(ancestor)) {
        continue;
      }

      GET_EXPECTED(st,
        t.get_reader(
          t1.select(t1.data, t1.consensus, t1.state, t1.order_no)
          .where(t1.id == ancestor)));
      GET_EXPECTED(st_result, st.execute());
      if (!st_result) {
        return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      if(orm::transaction_log_record_dbo::state_t::processed != t1.state.get(st)) {
        return vds::make_unexpected<std::runtime_error>("Invalid data");
      }

      if (t1.order_no.get(st) < min_order || t1.consensus.get(st)) {
        CHECK_EXPECTED(t.execute(
          t1.update(t1.state = orm::transaction_log_record_dbo::state_t::leaf)
          .where(t1.id == ancestor)));
        processed.emplace(ancestor);
        continue;
      }
    
      not_processed[ancestor] = t1.data.get(st);
    }
  }

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::process_records(const service_provider * sp, database_transaction & t, const transaction_block & block)
{
  std::stack<std::function<expected<void>()>> undo_actions;
  GET_EXPECTED(completed, block.walk_messages(
    [sp, &t, &undo_actions, &block](const payment_transaction & message) -> expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
    [sp, &t, &undo_actions, &block](const channel_message & message) -> expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
    [sp, &t, &undo_actions, &block](const create_user_transaction & message) -> expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
    [sp, &t, &undo_actions, &block](const node_add_transaction & message) -> expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
    [sp, &t, &undo_actions, &block](const create_wallet_transaction & message)->expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
      [sp, &t, &undo_actions, &block](const asset_issue_transaction & message)->expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    },
      [sp, &t, &undo_actions, &block](const store_block_transaction& message)->expected<bool> {
      GET_EXPECTED(result, apply_record(sp, t, message, block));
      if (result) {
        undo_actions.push([sp, &t, message, &block]() {return undo_record(sp, t, message, block); });
      }
      return result;
    }
    ));

  if (!completed) {
    while (!undo_actions.empty()) {
      CHECK_EXPECTED(undo_actions.top()());
      undo_actions.pop();
    }
  }

  return completed;
}

vds::expected<void> vds::transactions::transaction_log::consensus_records(
  const service_provider * sp,
  database_transaction & t,
  const transaction_block & block)
{
  CHECK_EXPECTED(block.walk_messages(
    [sp, &t, &block](const payment_transaction & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
    [sp, &t, &block](const channel_message & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
    [sp, &t, &block](const create_user_transaction & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
    [sp, &t, &block](const node_add_transaction & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
    [sp, &t, &block](const create_wallet_transaction & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
    [sp, &t, &block](const asset_issue_transaction & message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    },
      [sp, &t, &block](const store_block_transaction& message)->expected<bool> {
      CHECK_EXPECTED(consensus_record(sp, t, message, block));
      return true;
    }
    ));

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_records(
  const service_provider * sp,
  database_transaction & t,
  const transaction_block & block)
{
  std::stack<lambda_holder_t<expected<void>, const service_provider *, database_transaction &, const transaction_block&>> to_process;

  CHECK_EXPECTED(block.walk_messages(
    [&to_process](const payment_transaction & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const channel_message & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const create_user_transaction & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const node_add_transaction & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const create_wallet_transaction & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const asset_issue_transaction & message)->expected<bool> {
      to_process.emplace([message](const service_provider * sp, database_transaction & t, const transaction_block& block) { return undo_record(sp, t, message, block); });
      return true;
    },
    [&to_process](const store_block_transaction & message)->expected<bool> {
    to_process.emplace([message](const service_provider* sp, database_transaction& t, const transaction_block& block) { return undo_record(sp, t, message, block); });
    return true;
  }
  ));

  while (!to_process.empty()) {
    CHECK_EXPECTED(to_process.top()(sp, t, block));
    to_process.pop();
  }

  return expected<void>();
}


vds::expected<void> vds::transactions::transaction_log::invalid_block(
  const service_provider * sp,
  class database_transaction &t,
  const const_data_buffer & block_id,
  bool value) {

  orm::transaction_log_record_dbo t1;
  CHECK_EXPECTED(
    t.execute(
      t1
      .update(t1.state = (value ? orm::transaction_log_record_dbo::state_t::invalid : orm::transaction_log_record_dbo::state_t::validated))
      .where(t1.id == block_id)));

  std::set<const_data_buffer> followers;
  orm::transaction_log_hierarchy_dbo t4;
  GET_EXPECTED(st, t.get_reader(t4.select(t4.follower_id).where(t4.id == block_id)));
  WHILE_EXPECTED (st.execute()) {
    const auto follower_id = t4.follower_id.get(st);
    if (follower_id) {
      followers.emplace(follower_id);
    }
  }
  WHILE_EXPECTED_END()

  for (const auto &p : followers) {

    GET_EXPECTED_VALUE(st, t.get_reader(t1.select(t1.state, t1.data, t1.consensus).where(t1.id == p)));
    GET_EXPECTED(st_execute, st.execute());
    if (!st_execute
      || t1.state.get(st) == orm::transaction_log_record_dbo::state_t::leaf
      || t1.state.get(st) == orm::transaction_log_record_dbo::state_t::processed
      || t1.consensus.get(st)) {
      return vds::make_unexpected<std::runtime_error>("Invalid data");
    }

    CHECK_EXPECTED(invalid_block(sp, t, p, value));
  }

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::check_consensus(
  database_read_transaction& t,
  const const_data_buffer & log_id) {

  orm::transaction_log_vote_request_dbo t2;

  db_value<int> total_count;
  GET_EXPECTED(st, t.get_reader(
    t2.select(db_count(t2.owner).as(total_count))
    .where(t2.id == log_id && t2.new_member == false)));
  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return vds::make_unexpected<std::runtime_error>("Invalid data");
  }

  const auto tc = total_count.get(st);
  if (0 == tc) {
    return true;
  }

  db_value<int> appoved_count;
  GET_EXPECTED_VALUE(st, t.get_reader(
    t2.select(db_count(t2.owner).as(appoved_count))
    .where(t2.id == log_id && t2.approved == true && t2.new_member == false)));
  GET_EXPECTED_VALUE(st_execute, st.execute());
  if (!st_execute) {
    return vds::make_unexpected<std::runtime_error>("Invalid data");
  }

  auto ac = appoved_count.get(st);
  
  return (ac > tc / 2);
}


vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const payment_transaction & message,
  const transaction_block& block)
{
  sp->get<logger>()->trace(
    "DataCoin",
    "%s, apply payment %s[%s]->%s %d",
    base64::from_bytes(block.id()).c_str(),
    base64::from_bytes(message.source_wallet).c_str(),
    base64::from_bytes(message.source_transaction).c_str(),
    base64::from_bytes(message.target_wallet).c_str(),
    message.value);
  orm::wallet_dbo wt;
  GET_EXPECTED(st, t.get_reader(
    wt
    .select(wt.public_key)
    .where(
      wt.id == message.source_wallet)));

  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return false;
  }

  GET_EXPECTED(key, asymmetric_public_key::parse_der(wt.public_key.get(st)));
  GET_EXPECTED(signature_data, message.signature_data());
  GET_EXPECTED(signature_ok, asymmetric_sign_verify::verify(hash::sha256(), key, message.signature, signature_data));
  if (!signature_ok) {
    return false;
  }

  vds::orm::datacoin_balance_dbo t1;

  GET_EXPECTED_VALUE(st, t.get_reader(
    t1
    .select(t1.proposed_balance, t1.confirmed_balance)
    .where(
      t1.owner == message.source_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == message.source_transaction
      )));
  
  GET_EXPECTED_VALUE(st_execute, st.execute());
  if (!st_execute) {
    return false;
  }

  const auto balance = t1.proposed_balance.get(st);
  if (balance < message.value) {
    return false;
  }

  CHECK_EXPECTED(t.execute(
    t1.update(t1.proposed_balance = balance - message.value)
    .where(
      t1.owner == message.source_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == message.source_transaction
      )));

  CHECK_EXPECTED(t.execute(
    t1.insert(
      t1.proposed_balance = message.value,
      t1.confirmed_balance = 0,
      t1.owner = message.target_wallet,
      t1.issuer = message.issuer,
      t1.currency = message.currency,
      t1.source_transaction = block.id()
      )));

  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const payment_transaction & message,
  const transaction_block& block)
{
  sp->get<logger>()->trace(
    "DataCoin",
    "%s, consensus payment %s[%s]->%s %d",
    base64::from_bytes(block.id()).c_str(),
    base64::from_bytes(message.source_wallet).c_str(),
    base64::from_bytes(message.source_transaction).c_str(),
    base64::from_bytes(message.target_wallet).c_str(),
    message.value);

  vds::orm::datacoin_balance_dbo t1;

  GET_EXPECTED(st, t.get_reader(
    t1
    .select(t1.confirmed_balance)
    .where(
      t1.owner == message.source_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == message.source_transaction
      )));

  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return make_unexpected<std::runtime_error>("Database state is corrupted");
  }

  auto confirmed_balance = t1.confirmed_balance.get(st);
  if (confirmed_balance < message.value) {
    return make_unexpected<std::runtime_error>("Database state is corrupted");
  }

  CHECK_EXPECTED(t.execute(
    t1.update(
      t1.confirmed_balance = confirmed_balance - message.value
    )
    .where(
      t1.owner == message.source_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == message.source_transaction
      )));
   

  GET_EXPECTED_VALUE(st, t.get_reader(
    t1
    .select(t1.confirmed_balance)
    .where(
      t1.owner == message.target_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id()
      )));

  GET_EXPECTED_VALUE(st_execute, st.execute());
  if (!st_execute) {
    return make_unexpected<std::runtime_error>("Database state is corrupted");
  }

  confirmed_balance = t1.confirmed_balance.get(st);
  CHECK_EXPECTED(t.execute(
    t1.update(
      t1.confirmed_balance = confirmed_balance + message.value
    )
    .where(
      t1.owner == message.target_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id()
      )));

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const payment_transaction & message,
  const transaction_block& block)
{
  sp->get<logger>()->trace(
    "DataCoin",
    "%s, undo payment %s[%s]->%s %d",
    base64::from_bytes(block.id()).c_str(),
    base64::from_bytes(message.source_wallet).c_str(),
    base64::from_bytes(message.source_transaction).c_str(),
    base64::from_bytes(message.target_wallet).c_str(),
    message.value);

  vds::orm::datacoin_balance_dbo t1;

  GET_EXPECTED(st, t.get_reader(
    t1
    .select(t1.proposed_balance)
    .where(
      t1.owner == message.source_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == message.source_transaction
      )));

  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    auto balance = t1.proposed_balance.get(st);
    CHECK_EXPECTED(t.execute(
      t1.update(t1.proposed_balance = balance + message.value)
      .where(
        t1.owner == message.source_wallet
        && t1.issuer == message.issuer
        && t1.currency == message.currency
        && t1.source_transaction == message.source_transaction
        )));
  }
  else {
    CHECK_EXPECTED(t.execute(
      t1.insert(
        t1.proposed_balance = message.value,
        t1.owner = message.source_wallet,
        t1.issuer = message.issuer,
        t1.currency = message.currency,
        t1.source_transaction = message.source_transaction
        )));
  }

  CHECK_EXPECTED(t.execute(
    t1.delete_if(
      t1.owner == message.target_wallet
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id()
    )));

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const channel_message & message,
  const transaction_block& block)
{
  orm::channel_message_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.insert(
      t1.block_id = block.id(),
      t1.channel_id = message.channel_id(),
      t1.read_id = message.read_id(),
      t1.write_id = message.write_id(),
      t1.crypted_key = message.crypted_key(),
      t1.crypted_data = message.crypted_data(),
      t1.signature = message.signature()
    )));
  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const channel_message & message,
  const transaction_block& block)
{
  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const channel_message & message,
  const transaction_block& block)
{
  orm::channel_message_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.delete_if(
      t1.block_id == block.id()
      && t1.channel_id == message.channel_id()
      && t1.read_id == message.read_id()
      && t1.write_id == message.write_id()
      && t1.crypted_key == message.crypted_key()
      && t1.signature == message.signature()
    )));
  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const create_user_transaction & message,
  const transaction_block& block)
{
  GET_EXPECTED(id, message.user_public_key->fingerprint());
  GET_EXPECTED(public_key, message.user_public_key->der());

  orm::member_user_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.insert(
      t1.id = id,
      t1.public_key = public_key)));
  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const create_user_transaction & message,
  const transaction_block& block)
{
  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const create_user_transaction & message,
  const transaction_block& block)
{
  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const node_add_transaction & message,
  const transaction_block& block)
{
  GET_EXPECTED(node_id, message.node_public_key->fingerprint());
  GET_EXPECTED(public_key, message.node_public_key->der());

  orm::node_info_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.last_activity).where(t1.node_id == node_id)));
  GET_EXPECTED(st_execute, st.execute());
  if (st_execute) {
    if (t1.last_activity.get(st) < block.time_point()) {
      CHECK_EXPECTED(t.execute(t1.update(t1.last_activity = block.time_point()).where(t1.node_id == node_id)));
    }
  }
  else {
    CHECK_EXPECTED(t.execute(
      t1.insert(
        t1.node_id = node_id,
        t1.public_key = public_key,
        t1.last_activity = block.time_point()
      )));
  }


  orm::transaction_log_vote_request_dbo t2;
  CHECK_EXPECTED(t.execute(
    t2.insert(
      t2.id = block.id(),
      t2.owner = node_id,
      t2.approved = false,
      t2.new_member = true)));

  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const node_add_transaction & message,
  const transaction_block& block)
{
  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const node_add_transaction & message,
  const transaction_block& block)
{
  GET_EXPECTED(node_id, message.node_public_key->fingerprint());

  orm::node_info_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.delete_if(
      t1.node_id == node_id)));

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const create_wallet_transaction & message,
  const transaction_block& block)
{
  GET_EXPECTED(public_key, asymmetric_public_key::parse_der(message.public_key));
  GET_EXPECTED(wallet_id, public_key.fingerprint());

  orm::wallet_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.insert(
      t1.id = wallet_id,
      t1.public_key = message.public_key
    )));
  
  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const create_wallet_transaction & message,
  const transaction_block& block)
{
  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const create_wallet_transaction & message,
  const transaction_block& block)
{
  GET_EXPECTED(public_key, asymmetric_public_key::parse_der(message.public_key));
  GET_EXPECTED(wallet_id, public_key.fingerprint());

  orm::wallet_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.delete_if(
      t1.id == wallet_id)));

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider * sp,
  database_transaction & t,
  const asset_issue_transaction & message,
  const transaction_block& block)
{
  orm::member_user_dbo mu;
  GET_EXPECTED(st, t.get_reader(
    mu
    .select(mu.public_key)
    .where(
      mu.id == message.issuer)));

  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return false;
  }

  GET_EXPECTED(key, asymmetric_public_key::parse_der(mu.public_key.get(st)));
  GET_EXPECTED(signature_data, message.signature_data());
  GET_EXPECTED(signature_ok, asymmetric_sign_verify::verify(hash::sha256(), key, message.signature, signature_data));
  if (!signature_ok) {
    return false;
  }

  vds::orm::datacoin_balance_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.insert(
      t1.proposed_balance = message.value,
      t1.confirmed_balance = 0,
      t1.owner = message.wallet_id,
      t1.issuer = message.issuer,
      t1.currency = message.currency,
      t1.source_transaction = block.id()
    )));

  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(
  const service_provider * sp,
  database_transaction & t,
  const asset_issue_transaction & message,
  const transaction_block& block)
{
  vds::orm::datacoin_balance_dbo t1;

  GET_EXPECTED(st, t.get_reader(
    t1
    .select(t1.proposed_balance, t1.confirmed_balance)
    .where(
      t1.owner == message.wallet_id
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id()
      )));

  GET_EXPECTED(st_execute, st.execute());
  if (!st_execute) {
    return make_unexpected<std::runtime_error>("Database state is corrupted");
  }

  if (t1.proposed_balance.get(st) != message.value || t1.confirmed_balance.get(st) != 0) {
    return make_unexpected<std::runtime_error>("Database state is corrupted");
  }
  
  CHECK_EXPECTED(t.execute(
    t1.update(
      t1.proposed_balance = message.value,
      t1.confirmed_balance = message.value)
    .where(
      t1.owner == message.wallet_id
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id()
    )));

  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(
  const service_provider * sp,
  database_transaction & t,
  const asset_issue_transaction & message,
  const transaction_block& block)
{
  vds::orm::datacoin_balance_dbo t1;
  CHECK_EXPECTED(t.execute(
    t1.delete_if(
      t1.owner == message.wallet_id
      && t1.issuer == message.issuer
      && t1.currency == message.currency
      && t1.source_transaction == block.id())));

  return expected<void>();
}

vds::expected<bool> vds::transactions::transaction_log::apply_record(
  const service_provider* sp,
  database_transaction& t,
  const store_block_transaction& message,
  const transaction_block& block)
{
  GET_EXPECTED(root_folder, persistence::current_user(sp));
  foldername tmp_folder(root_folder, "tmp");

  auto client = sp->get<dht::network::client>();
  if (block.write_public_key_id() == client->current_node_id()) {
    for (const auto& p : message.replicas) {
      auto append_path = base64::from_bytes(p);
      str_replace(append_path, '+', '#');
      str_replace(append_path, '/', '_');

      CHECK_EXPECTED((*client)->save_data(
        sp,
        t,
        p,
        filename(tmp_folder, append_path),
        message.owner_id));

      orm::chunk_tmp_data_dbo t1;
      CHECK_EXPECTED(t.execute(t1.delete_if(t1.object_id == p)));
    }
  }

  for (uint16_t index = 0; index < message.replicas.size(); ++index) {
    orm::chunk_replica_data_dbo t1;
    CHECK_EXPECTED(t.execute(
      t1.insert(
        t1.object_hash = message.object_id,
        t1.replica = index,
        t1.replica_hash = message.replicas[index],
        t1.replica_size = message.replica_size
      )
    ));

    orm::sync_replica_map_dbo t2;
    CHECK_EXPECTED(t.execute(
      t2.insert(
        t2.replica_hash = message.replicas[index],
        t2.node = block.write_public_key_id(),
        t2.last_access = std::chrono::system_clock::now()        
      )
    ));
  }

  return true;
}

vds::expected<void> vds::transactions::transaction_log::consensus_record(const service_provider* sp, database_transaction& t, const store_block_transaction& message, const transaction_block& block)
{
  return expected<void>();
}

vds::expected<void> vds::transactions::transaction_log::undo_record(const service_provider* sp, database_transaction& t, const store_block_transaction& message, const transaction_block& block)
{
  for (uint16_t index = 0; index < message.replicas.size(); ++index) {
    orm::chunk_replica_data_dbo t1;
    CHECK_EXPECTED(t.execute(
      t1.delete_if(
        t1.object_hash == message.object_id
        && t1.replica == index
        && t1.replica_hash == message.replicas[index]
      )
    ));

    orm::sync_replica_map_dbo t2;
    CHECK_EXPECTED(t.execute(
      t1.delete_if(
        t2.replica_hash == message.replicas[index]
        && t2.node == block.write_public_key_id()
      )
    ));
  }
  return expected<void>();
}
