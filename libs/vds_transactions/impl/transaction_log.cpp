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
      t1.order_no = block.order_no(),
      t1.time_point = block.time_point()));

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

  //Check ancestors
  orm::transaction_log_record_dbo t1;
  std::set<const_data_buffer> remove_leaf;
  bool is_invalid = false;
  for (const auto & ancestor : block.ancestors()) {
    auto st = t.get_reader(t1.select(t1.state,t1.order_no,t1.time_point).where(t1.id == ancestor));
    if (!st.execute()) {
      return;
    }
    else {
      if(t1.order_no.get(st) >= block.order_no() || t1.time_point.get(st) > block.time_point()) {
        is_invalid = true;
        vds_assert(false);
      }
      else {
        switch (static_cast<orm::transaction_log_record_dbo::state_t>(t1.state.get(st))) {
        case orm::transaction_log_record_dbo::state_t::leaf: {
          remove_leaf.emplace(ancestor);
          break;
        }

        case orm::transaction_log_record_dbo::state_t::consensus:
        case orm::transaction_log_record_dbo::state_t::processed:
        case orm::transaction_log_record_dbo::state_t::validated:
          break;

        case orm::transaction_log_record_dbo::state_t::invalid:
          is_invalid = true;
          break;

        default:
          throw std::runtime_error("Invalid program");
        }
      }
    }
  }


  if(is_invalid) {
    update_consensus(sp, t, block_data);
    t.execute(
      t1.update(
        t1.state = orm::transaction_log_record_dbo::state_t::invalid)
      .where(t1.id == block.id()));
    return;
  }

  auto state = orm::transaction_log_record_dbo::state_t::leaf;
  try {
    auto datacoin_state = transaction_record_state::load(t, block);
    datacoin_state.save(t, block.write_cert_id(), block.id());
    //apply_block
  }
  catch(...) {
    state = orm::transaction_log_record_dbo::state_t::invalid;
  }

  update_consensus(sp, t, block_data);
  t.execute(t1.update(t1.state = state).where(t1.id == block.id()));

  if (orm::transaction_log_record_dbo::state_t::leaf == state) {
    for (const auto & p : remove_leaf) {

      orm::transaction_log_vote_request_dbo t2;
      db_value<int> appoved_count;
      auto st = t.get_reader(
        t2.select(db_count(t2.owner).as(appoved_count)).where(t2.id == p && t2.is_appoved == true).group_by(t2.owner));
      auto ac = st.execute() ? appoved_count.get(st) : 0;

      db_value<int> total_count;
      st = t.get_reader(t2.select(db_count(t2.owner).as(total_count)).where(t2.id == p).group_by(t2.owner));
      if (!st.execute()) {
        throw std::runtime_error("Invalid data");
      }
      auto tc = total_count.get(st);

      if (ac > tc / 2) {
        t.execute(
          t2.delete_if(t2.id == p));

        t.execute(
          t1.update(
            t1.state = orm::transaction_log_record_dbo::state_t::consensus)
          .where(t1.id == p));
      }
      else {
        t.execute(
          t1.update(
            t1.state = orm::transaction_log_record_dbo::state_t::processed)
          .where(t1.id == p));
      }
    }
  }

  //process followers
  std::set<const_data_buffer> followers;
  orm::transaction_log_hierarchy_dbo t4;
  auto st = t.get_reader(t4.select(t4.follower_id).where(t4.id == block.id()));
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

      if (orm::transaction_log_record_dbo::state_t::processed == t1.state.get(st)) {
        auto ancestor_data = t1.data.get(st);

        orm::transaction_log_vote_request_dbo t2;
        t.execute(
          t2.update(t2.is_appoved = true)
          .where(t2.id == ancestor && t2.owner == leaf_owner));

        db_value<int> appoved_count;
        st = t.get_reader(t2.select(db_count(t2.owner).as(appoved_count)).where(t2.id == ancestor && t2.is_appoved == true).group_by(t2.owner));
        if (!st.execute()) {
          throw std::runtime_error("Invalid data");
        }
        auto ac = appoved_count.get(st);

        db_value<int> total_count;
        st = t.get_reader(t2.select(db_count(t2.owner).as(total_count)).where(t2.id == ancestor).group_by(t2.owner));
        if (!st.execute()) {
          throw std::runtime_error("Invalid data");
        }
        auto tc = total_count.get(st);

        if(ac > tc / 2) {
          t.execute(
            t2.delete_if(t2.id == ancestor));

          t.execute(
            t1.update(
              t1.state = orm::transaction_log_record_dbo::state_t::consensus)
            .where(t1.id == ancestor));
        }

        if(block.write_cert_id() != leaf_owner){
          not_processed[ancestor] = ancestor_data;
        }
      }
    }
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
