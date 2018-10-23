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

void vds::transactions::transaction_log::save(
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
}

void vds::transactions::transaction_log::process_block(
  const service_provider* sp,
  database_transaction& t,
  const const_data_buffer& block_data) {
  transaction_block block(block_data);

  //Check ancestors
  orm::transaction_log_record_dbo t1;
  std::set<const_data_buffer> remove_leaf;
  for (const auto & ancestor : block.ancestors()) {
    auto st = t.get_reader(t1.select(t1.state).where(t1.id == ancestor));
    if (!st.execute()) {
      return;
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
        return;;

      default:
        throw std::runtime_error("Invalid program");
      }
    }
  }

  auto state = orm::transaction_log_record_dbo::state_t::leaf;
  try {
    //apply_block
  }
  catch(...) {
    state = orm::transaction_log_record_dbo::state_t::invalid;
  }

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
