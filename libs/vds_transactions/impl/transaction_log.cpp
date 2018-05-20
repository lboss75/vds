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
#include "transaction_log_unknown_record_dbo.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "vds_exceptions.h"
#include "logger.h"
#include "certificate_chain_dbo.h"
#include "include/transaction_state_calculator.h"

void vds::transactions::transaction_log::save(
	const service_provider & sp,
	database_transaction & t,
  const const_data_buffer & refer_id,
	const const_data_buffer & block_data)
{
  transaction_block block(block_data);

  if(block.exists(t)) {
    return;
  }

  std::set<const_data_buffer> remove_leaf;
  bool ancestors_exist = true;
  bool ancestor_invalid = false;
  orm::transaction_log_record_dbo t1;
  for (const auto & ancestor : block.ancestors()) {
    auto st = t.get_reader(t1.select(t1.state).where(t1.id == base64::from_bytes(ancestor)));
    if (!st.execute()) {
      ancestors_exist = false;
      orm::transaction_log_unknown_record_dbo t4;
      t.execute(t4.insert_or_ignore(
        t4.id = base64::from_bytes(ancestor),
        t4.refer_id = refer_id,
        t4.follower_id = base64::from_bytes(block.id())
      ));
    }
    else {
      switch (static_cast<orm::transaction_log_record_dbo::state_t>(t1.state.get(st))) {
      case orm::transaction_log_record_dbo::state_t::leaf: {
        remove_leaf.emplace(ancestor);
        break;
      }

      case orm::transaction_log_record_dbo::state_t::processed:
        break;

      case orm::transaction_log_record_dbo::state_t::invalid:
        ancestor_invalid = true;
        break;

      default:
        throw std::runtime_error("Invalid program");
      }
    }
  }

  if (!ancestors_exist) {
    return;
  }

  certificate write_cert;
  if (block.ancestors().empty()) {
    block.walk_messages(
      [&write_cert](const root_user_transaction & message)->bool{
      write_cert = message.user_cert();
      return true;
    });
  }

  if (!write_cert) {
    orm::certificate_chain_dbo t2;
    auto st = t.get_reader(t2.select(t2.cert).where(t2.id == block.write_cert_id()));
    if (st.execute()) {
      write_cert = certificate::parse_der(t2.cert.get(st));
    }
  }

  if(!write_cert){
    sp.get<logger>()->warning(
      ThisModule,
      sp,
      "Invalid certificate %s for block %s",
      block.write_cert_id().c_str(),
      base64::from_bytes(block.id()).c_str());
  }
  else {
    if(!block.validate(write_cert)){
      sp.get<logger>()->warning(
          ThisModule,
          sp,
          "Invalid signature record %s",
          base64::from_bytes(block.id()).c_str());
      return;
    }
  }

  if(ancestor_invalid) {
    t.execute(
      t1.insert(
        t1.id = base64::from_bytes(block.id()),
        t1.data = block_data,
        t1.state = static_cast<int>(orm::transaction_log_record_dbo::state_t::invalid),
        t1.order_no = block.order_no()));
  }
  else {
    update_consensus(t, block, block.write_cert_id());

    //try {
    auto state = transaction_state_calculator::calculate(t, block.ancestors(), block.order_no());
    state.apply(block);
    //}
    //catch (const transactions::transaction_source_not_found_error & ex) {
    //  //
    //}
    //catch (const transactions::transaction_lack_of_funds & ex) {

    //}

    t.execute(
      t1.insert(
        t1.id = base64::from_bytes(block.id()),
        t1.data = block_data,
        t1.state = static_cast<int>(orm::transaction_log_record_dbo::state_t::leaf),
        t1.order_no = block.order_no(),
        t1.state_data = state.serialize()));

    for (const auto & p : remove_leaf) {
      t.execute(
        t1.update(
          t1.state = static_cast<int>(orm::transaction_log_record_dbo::state_t::processed))
        .where(t1.id == base64::from_bytes(p)));
    }
  }

  //process followers
  std::set<const_data_buffer> followers;
  orm::transaction_log_unknown_record_dbo t4;
  auto st = t.get_reader(t4.select(t4.follower_id).where(t4.id == base64::from_bytes(block.id())));
  while (st.execute()) {
    followers.emplace(base64::to_bytes(t4.id.get(st)));
  }

  for (const auto &p : followers) {
    sp.get<logger>()->trace(
      ThisModule,
      sp,
      "Apply follower %s for %s",
      base64::from_bytes(p).c_str(),
      base64::from_bytes(block.id()).c_str());

    st = t.get_reader(t1.select(t1.data).where(t1.id == base64::from_bytes(p)));
    if (!st.execute()) {
      throw std::runtime_error("Invalid data");
    }

    t.execute(
      t4.delete_if(
        t4.id == base64::from_bytes(block.id())
        && t4.follower_id == base64::from_bytes(p)));
  }
}

void vds::transactions::transaction_log::update_consensus(
  database_transaction& t,
  const transactions::transaction_block& block,
  const std::string & account) {

  orm::transaction_log_record_dbo t1;
  for (const auto & ancestor_id : block.ancestors()) {
    auto st = t.get_reader(t1.select(t1.data, t1.state_data).where(t1.id == base64::from_bytes(ancestor_id)));
    if (!st.execute()) {
      throw std::runtime_error("Invalid program");
    }
    else {
      const auto state_data = t1.state_data.get(st);
      binary_deserializer state_deserializer(state_data);
      transactions::transaction_record_state state(state_deserializer);

      if(state.update_consensus(account)) {
        transactions::transaction_block ancestor(t1.data.get(st));
        update_consensus(t, ancestor, account);
      }
    }
  }
}