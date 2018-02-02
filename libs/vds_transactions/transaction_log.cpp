/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "transaction_log.h"
#include "private/transaction_log_p.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "transaction_block.h"
#include "transaction_log_unknown_record_dbo.h"
#include "transactions/channel_add_writer_transaction.h"
#include "transactions/device_user_add_transaction.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/user_channel_create_transaction.h"
#include "run_configuration_dbo.h"
#include "vds_debug.h"
#include "transaction_log_record_dbo.h"
#include "encoding.h"
#include "user_manager.h"
#include "member_user.h"
#include "cert_control.h"
#include "vds_exceptions.h"
#include "encoding.h"
#include "cert_control.h"
#include "vds_debug.h"

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
  sp.get<logger>()->trace(
      ThisModule,
      sp,
      "%s is state %s",
      base64::from_bytes(block_id).c_str(),
      orm::transaction_log_record_dbo::str(state).c_str());

  vds_assert(orm::transaction_log_record_dbo::state_t::none != state);

  t.execute(t2.insert(
      t2.id = base64::from_bytes(block_id),
      t2.data = block_data,
      t2.state = (uint8_t)state));

  if(orm::transaction_log_record_dbo::state_t::processed == state) {
    orm::transaction_log_unknown_record_dbo t4;
    for (auto &p : followers) {
      sp.get<logger>()->trace(
          ThisModule,
          sp,
          "Apply follower %s for %s",
          base64::from_bytes(p).c_str(),
          base64::from_bytes(block_id).c_str());

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

  sp.get<logger>()->trace(
      ThisModule,
      sp,
      "%s in state %s",
      base64::from_bytes(block_id).c_str(),
      orm::transaction_log_record_dbo::str(state).c_str());

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
  guid read_cert_id;
  guid write_cert_id;
  std::set<const_data_buffer> ancestors;
  const_data_buffer body;
  const_data_buffer crypted_key;
  const_data_buffer signature;

  binary_deserializer crypted(block_data);
  crypted
      >> read_cert_id
      >> write_cert_id
      >> ancestors
      >> body
      >> crypted_key;

  auto crypted_size = block_data.size() - crypted.size();
  crypted
      >> signature;

  auto user_mng = sp.get<user_manager>();
  auto common_channel = user_mng->get_common_channel(sp);
  //Validate
  bool is_validated = false;
  auto validate_cert = user_mng->get_channel_write_cert(sp, common_channel.id(), write_cert_id);
  if (validate_cert) {
    if (!asymmetric_sign_verify::verify(hash::sha256(), validate_cert.public_key(), signature, block_data.data(), crypted_size)) {
      throw vds_exceptions::signature_validate_error();
    }

    is_validated = true;
  }

  //
  bool is_ready = true;
  for(auto & id : ancestors){
    auto id_str = base64::from_bytes(id);
    orm::transaction_log_record_dbo t3;
    auto st = t.get_reader(t3.select(t3.state).where(t3.id == id_str));
    if(!st.execute()) {
      is_ready = false;

      orm::transaction_log_unknown_record_dbo t4;
      st = t.get_reader(
          t4.select(t4.id)
              .where(t4.id == id_str
                     && t4.follower_id == base64::from_bytes(block_id)));
      if (!st.execute()) {
        sp.get<logger>()->trace(
            ThisModule,
            sp,
            "Dependency %s has been registered as unknown",
            id_str.c_str());
        t.execute(t4.insert(
            t4.id = id_str,
            t4.follower_id = base64::from_bytes(block_id)));
      }
      else {
        sp.get<logger>()->trace(
            ThisModule,
            sp,
            "Dependency %s has been registered as unknown already",
            id_str.c_str());
      }
    } else {
      auto state = (orm::transaction_log_record_dbo::state_t)t3.state.get(st);
      sp.get<logger>()->trace(
          ThisModule,
          sp,
          "Dependency %s already exists in state %s",
          id_str.c_str(),
          orm::transaction_log_record_dbo::str(state).c_str());
      if(orm::transaction_log_record_dbo::state_t::leaf == state){
        t.execute(
            t3.update(
                    t3.state = (int)orm::transaction_log_record_dbo::state_t::processed)
                .where(t3.id == id_str));
      }
    }

  }

  if(is_ready){
    //Decrypt
    auto user_mng = sp.get<user_manager>();

    asymmetric_private_key device_private_key;
    auto device_user = user_mng->get_current_device(sp, device_private_key);

    auto common_private_key = user_mng->get_common_channel_read_key(sp, read_cert_id);

    auto key_data = common_private_key.decrypt(crypted_key);
    auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);

    auto data = symmetric_decrypt::decrypt(key, body);
    apply_message(sp, t, data);

    if(!is_validated){
      auto validate_cert = user_mng->get_channel_write_cert(sp, common_channel.id(), write_cert_id);
      if (!validate_cert
          || !asymmetric_sign_verify::verify(hash::sha256(), validate_cert.public_key(), signature, block_data.data(), crypted_size)) {
          throw vds_exceptions::signature_validate_error();
      }
    }

    orm::transaction_log_unknown_record_dbo t4;
    auto st = t.get_reader(
        t4.select(t4.follower_id)
            .where(t4.id == base64::from_bytes(block_id)));
    while(st.execute()){
      followers.push_back(base64::to_bytes(t4.follower_id.get(st)));
    }

    orm::transaction_log_record_dbo t2;
    if(followers.empty()) {
      return orm::transaction_log_record_dbo::state_t::leaf;
    } else {
      return orm::transaction_log_record_dbo::state_t::processed;
    }
  }
  else {
    return is_validated
           ? orm::transaction_log_record_dbo::state_t::validated
           : orm::transaction_log_record_dbo::state_t::none;
  }
}

void vds::transaction_log::apply_message(
    const vds::service_provider &sp,
    database_transaction &t,
    const const_data_buffer &block_data) {
  binary_deserializer s(block_data);

  while(0 < s.size()) {
    uint8_t command_id;
    s >> command_id;
    switch ((transactions::transaction_id)command_id) {
      case transactions::transaction_id::root_user_transaction: {
        transactions::root_user_transaction message(s);
        message.apply(sp, t);
        break;
      }

      case transactions::transaction_id::channel_create_transaction: {
        transactions::channel_create_transaction message(s);
        message.apply(sp, t);
        break;
      }

      case transactions::transaction_id::channel_message_transaction: {
        transactions::channel_message_transaction message(s);
        message.apply(sp, t);
        break;
      }

      case transactions::transaction_id::user_channel_create_transaction: {
        transactions::user_channel_create_transaction message(s);
        message.apply(sp, t);
        break;
      }

      case transactions::transaction_id::device_user_add_transaction: {
        transactions::device_user_add_transaction message(s);
        message.apply(sp, t);
        break;
      }

      default:
        throw std::runtime_error("Invalid data");
    }
  }
}

////////////////////////////////////////////
void vds::transactions::channel_message_transaction::apply(
    const service_provider & sp,
    database_transaction & t) const {

  dbo::channel_message t1;
  t.execute(t1.insert(
      t1.channel_id = this->channel_id_,
      t1.message_id = this->message_id_,
      t1.read_cert_id = this->read_cert_id_,
      t1.write_cert_id = this->write_cert_id_,
      t1.message = this->data_,
      t1.signature = this->signature_));

  auto user_mng = sp.get<user_manager>();
  user_mng->apply_channel_message(
	  sp,
	  this->channel_id_,
	  this->message_id_,
	  this->read_cert_id_,
	  this->write_cert_id_,
	  this->data_,
	  this->signature_);
}

