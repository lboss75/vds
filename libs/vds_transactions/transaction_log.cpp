/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_log.h"
#include "private/transaction_log_p.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "transaction_block.h"
#include <set>
#include <transaction_log_unknown_record_dbo.h>
#include <transactions/channel_add_writer_transaction.h>
#include <transactions/device_user_add_transaction.h>
#include <transactions/channel_create_transaction.h>
#include "vds_debug.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "member_user.h"
#include "cert_control.h"
#include "certificate_dbo.h"
#include "vds_exceptions.h"
#include "encoding.h"

void vds::transaction_log::save(
	const service_provider & sp,
	database_transaction & t,
	const const_data_buffer & block_id,
	const const_data_buffer & block_data)
{
	orm::transaction_log_record_dbo t2;
	auto st = t.get_reader(t2.select(t2.state).where(t2.id == base64::from_bytes(block_id)));

	if (st.execute()) {
		return;//Already exists
	}

  std::list<const_data_buffer> followers;
  auto state = apply_block(sp, t, block_id, block_data, followers);
  vds_assert(orm::transaction_log_record_dbo::state_t::none != state);

  t.execute(t2.insert(
      t2.id = base64::from_bytes(block_id),
      t2.data = block_data,
      t2.state = (uint8_t)state));

  if(orm::transaction_log_record_dbo::state_t::processed == state) {
    orm::transaction_log_unknown_record_dbo t4;
    for (auto &p : followers) {
      t.execute(
          t4.delete_if(
              t4.id == base64::from_bytes(block_id)
              && t4.follower_id == base64::from_bytes(p)));

      apply_block(sp, t, p);
    }
  }
}

void vds::transaction_log::apply_block(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::const_data_buffer &block_id) {

  orm::transaction_log_record_dbo t2;
  auto st = t.get_reader(
      t2.select(t2.state, t2.data)
          .where(t2.id == base64::from_bytes(block_id)));
  if(!st.execute()){
    throw std::runtime_error("Invalid data");
  }
  auto state = (orm::transaction_log_record_dbo::state_t)t2.state.get(st);
  if(orm::transaction_log_record_dbo::state_t::validated != state){
    return;
  }

  auto block_data = t2.data.get(st);
  std::list<const_data_buffer> followers;
  state = apply_block(sp, t, block_id, block_data, followers);

  vds_assert(orm::transaction_log_record_dbo::state_t::processed != state);
  if(orm::transaction_log_record_dbo::state_t::none != state) {
    t.execute(t2.update(t2.state = (uint8_t) state)
                  .where(t2.id == base64::from_bytes(block_id)));
  }
}

vds::orm::transaction_log_record_dbo::state_t vds::transaction_log::apply_block(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::const_data_buffer &block_id,
    const vds::const_data_buffer &block_data,
    std::list<const_data_buffer> & followers) {
  //Parse block
  guid common_read_cert_id;
  guid write_cert_id;
  std::set<const_data_buffer> ancestors;
  const_data_buffer body;
  const_data_buffer crypted_key;
  const_data_buffer signature;

  binary_deserializer crypted(block_data);
  crypted
      >> common_read_cert_id
      >> write_cert_id
      >> ancestors
      >> body
      >> crypted_key;

  auto crypted_size = block_data.size() - crypted.size();
  crypted
      >> signature;

  //Validate
  bool is_validated = false;
  certificate_dbo t1;
  auto st = t.get_reader(t1.select(t1.cert).where(t1.id == write_cert_id));
  if (st.execute()) {
    auto cert = certificate::parse_der(t1.cert.get(st));
    if (!asymmetric_sign_verify::verify(hash::sha256(), cert.public_key(), signature, block_data.data(), crypted_size)) {
      throw vds_exceptions::signature_validate_error();
    }

    is_validated = true;
  }

  //
  bool is_ready = true;
  for(auto & id : ancestors){
    auto id_str = base64::from_bytes(id);
    orm::transaction_log_record_dbo t3;
    st = t.get_reader(t3.select(t3.id).where(t3.id == id_str));
    if(!st.execute()) {
      is_ready = false;

      orm::transaction_log_unknown_record_dbo t4;
      st = t.get_reader(
          t4.select(t4.id)
              .where(t4.id == id_str
                     && t4.follower_id == base64::from_bytes(block_id)));
      if (!st.execute()) {
        t.execute(t4.insert(
            t4.id = id_str,
            t4.follower_id = base64::from_bytes(block_id)));
      }
    }
  }

  if(is_validated) {
    if(is_ready){
      //Decript
      auto user_mng = sp.get<user_manager>();

      asymmetric_private_key device_private_key;
      auto device_user = user_mng->get_current_device(sp, t, device_private_key);

      auto common_private_key = user_mng->get_private_key(
          t,
          common_read_cert_id,
          cert_control::get_id(device_user.user_certificate()),
          device_private_key);

      auto key_data = common_private_key.decrypt(crypted_key);
      auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);

      auto data = symmetric_decrypt::decrypt(key, body);
      apply_block(sp, data);

      orm::transaction_log_unknown_record_dbo t4;
      st = t.get_reader(
          t4.select(t4.follower_id)
              .where(t4.id == base64::from_bytes(block_id)));
      while(st.execute()){
        followers.push_back(base64::to_bytes(t4.id.get(st)));
      }

      orm::transaction_log_record_dbo t2;
      if(followers.empty()) {
        return orm::transaction_log_record_dbo::state_t::leaf;
      } else {
        return orm::transaction_log_record_dbo::state_t::processed;
      }
    }
    else {
      return orm::transaction_log_record_dbo::state_t::validated;
    }
  }
  return orm::transaction_log_record_dbo::state_t::none;
}

static void apply_command(
    const vds::service_provider & sp,
    const vds::transactions::root_user_transaction &message) {

}

static void apply_command(
    const vds::service_provider & sp,
    const vds::transactions::channel_message_transaction & message){

}

static void apply_command(
    const vds::service_provider & sp,
    const vds::transactions::channel_create_transaction & message){

}

static void apply_command(
    const vds::service_provider & sp,
    const vds::transactions::device_user_add_transaction & message){
}

void vds::transaction_log::apply_block(
    const service_provider & sp,
    const const_data_buffer & block_data) {
  binary_deserializer s(block_data);

  while(0 < s.size()) {
    uint8_t command_id;
    s >> command_id;
    switch (command_id) {
      case transactions::root_user_transaction::message_id: {
        transactions::root_user_transaction message(s);
        apply_command(sp, message);
        break;
      }

      case transactions::channel_create_transaction::message_id: {
        transactions::channel_create_transaction message(s);
        apply_command(sp, message);
        break;
      }

      case transactions::channel_message_transaction::message_id: {
        transactions::channel_message_transaction message(s);
        apply_command(sp, message);
        break;
      }

      case transactions::device_user_add_transaction::message_id: {
        transactions::device_user_add_transaction message(s);
        apply_command(sp, message);
        break;
      }

      default:
        throw std::runtime_error("Invalid data");
    }
  }
}

