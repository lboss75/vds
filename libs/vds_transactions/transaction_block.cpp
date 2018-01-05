/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include <chunk_manager.h>
#include <transaction_log_record_relation_dbo.h>
#include "user_manager.h"
#include "transaction_block.h"
#include "symmetriccrypto.h"
#include "asymmetriccrypto.h"
#include "guid.h"
#include "transaction_context.h"
#include "cert_control.h"
#include "database_orm.h"
#include "chunk_data_dbo.h"
#include "transaction_log_record_dbo.h"
#include "transactions/channel_add_dependency_transaction.h"
#include "encoding.h"
#include "member_user.h"

vds::const_data_buffer
vds::transaction_block::sign(const class certificate &target_cert, const class guid &sign_cert_key_id,
                             const class asymmetric_private_key &sign_cert_key) {

  auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  binary_serializer result;
  result
      << sign_cert_key_id
      << cert_control::get_id(target_cert)
      << target_cert.public_key().encrypt(skey.serialize())
      << symmetric_encrypt::encrypt(skey, this->s_.data());

  result << asymmetric_sign::signature(
      hash::sha256(),
      sign_cert_key,
      result.data());

  return result.data();
}

vds::const_data_buffer vds::transaction_block::unpack_block(
    service_provider & sp,
    const vds::const_data_buffer &data,
    const std::function<certificate(const guid &)> &get_cert_handler,
    const std::function<asymmetric_private_key(const guid &)> & get_key_handler) {
  binary_deserializer s(data);

  guid sign_cert_id;
  guid target_cert_id;
  const_data_buffer skey_data;
  const_data_buffer crypted_data;
  s >> sign_cert_id >> target_cert_id >> skey_data >> crypted_data;

  auto body_size = data.size() - s.size();

  const_data_buffer sign;
  s >> sign;

  if(0 != s.size()){
    throw std::runtime_error("Invalid data");
  }

  auto cert = get_cert_handler(sign_cert_id);

  if(!cert){
    throw std::runtime_error("Certificate " + sign_cert_id.str() + " is not found");
  }

  if(!asymmetric_sign_verify::verify(
      hash::sha256(),
      cert.public_key(),
      sign,
      data.data(), body_size)){
    throw std::runtime_error("Invalid data");
  }

  auto key = get_key_handler(target_cert_id);

  if(!key){
    throw std::runtime_error("Key " + target_cert_id.str() + " is not found");
  }

  auto decrypted = key.decrypt(skey_data);
  auto skey = symmetric_key::deserialize(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(decrypted));

  sp.set_property(
      service_provider::property_scope::local_scope,
      new transaction_context(sign_cert_id));

  return symmetric_decrypt::decrypt(skey, crypted_data);
}

void vds::transaction_block::pack(
    const service_provider & sp,
    vds::database_transaction &t,
    class member_user & owner,
    const asymmetric_private_key & owner_private_key) {

  std::list<std::string> ancestors;

  binary_serializer s;
  s << cert_control::get_id(owner.user_certificate());

  auto user_mng = sp.get<user_manager>();
  for(auto & p : this->channels_){
    auto channel_id = p.first;
    auto channel = user_mng->get_channel(t, channel_id);

    asymmetric_private_key channel_private_key;
    auto channel_write_cert = user_mng->get_channel_write_cert(
        sp,
        t,
        channel,
        owner.id(),
        channel_private_key);

    std::list<dependency_info> dependencies;
    orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(
        t1.select(t1.id, t1.key)
            .where(t1.channel_id == channel_id
            && t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
    while(st.execute()){
      auto id = t1.id.get(st);
      ancestors.push_back(id);
      dependencies.push_back(dependency_info {
          .id = base64::to_bytes(id),
          .key = t1.key.get(st)
      });
    }

    auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());
    s
        << channel_id
        << cert_control::get_id(channel_write_cert)
        << dependencies
        << channel_write_cert.public_key().encrypt(skey.serialize())
        << symmetric_encrypt::encrypt(skey, p.second.s_.data());
  }

  s << asymmetric_sign::signature(
      hash::sha256(),
      owner_private_key,
      s.data());

  auto block_info = chunk_manager::pack_block(s.data());

  for(auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    t.execute(
        t1.update(
            t1.id == ancestor
            && t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::pending));

    orm::transaction_log_record_relation_dbo t2;
    t.execute(t2.insert(
        t2.predecessor_id = ancestor,
        t2.follower_id = base64::from_bytes(block_info.id),
        t2.relation_type = (uint8_t)orm::transaction_log_record_relation_dbo::relation_type_t::week
    ));
  }

  orm::transaction_log_record_dbo t1;
  t.execute(
      t1.insert(
          t1.id = base64::from_bytes(block_info.id),
          t1.key = block_info.key,
          t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
}
