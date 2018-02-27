/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "chunk_manager.h"
#include "p2p_network.h"
#include "messages/channel_log_record.h"
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
#include "encoding.h"
#include "member_user.h"
#include "vds_debug.h"
#include "transaction_log.h"

std::map<vds::guid, vds::const_data_buffer> vds::transactions::transaction_block::save(
    const service_provider &sp,
    class vds::database_transaction & t,
    const std::function<void(
        const guid & channel_id,
        certificate & read_cert,
        certificate & write_cert,
        asymmetric_private_key & write_private_key)> & crypto_callback) const {
  std::map<guid, vds::const_data_buffer> result;
  for(auto & p : this->data_) {
    vds_assert(0 != p.second.size());

    certificate read_cert;
    certificate write_cert;
    asymmetric_private_key write_private_key;
    crypto_callback(p.first, read_cert, write_cert, write_private_key);

    std::set<const_data_buffer> ancestors;
    this->collect_dependencies(t, p.first, ancestors);

    auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

    binary_serializer crypted;
    crypted
      << p.first
      << cert_control::get_id(read_cert)
      << cert_control::get_id(write_cert)
      << ancestors
      << symmetric_encrypt::encrypt(key, p.second.data())
      << write_cert.public_key().encrypt(key.serialize());

    crypted << asymmetric_sign::signature(
        hash::sha256(),
        write_private_key,
        crypted.data());

    auto id = register_transaction(sp, t, p.first, crypted.data(), ancestors);
    on_new_transaction(sp, t, id, crypted.data());

    result[p.first] = id;
  }
  return result;
}

void vds::transactions::transaction_block::parse_block(
  const const_data_buffer & data,
  guid & channel_id,
  guid & read_cert_id,
  guid & write_cert_id,
  std::set<const_data_buffer> & ancestors,
  const_data_buffer & crypted_data,
  const_data_buffer & crypted_key,
  const_data_buffer & signature) {

  binary_deserializer crypted(data);
  crypted
    >> channel_id
    >> read_cert_id
    >> write_cert_id
    >> ancestors
    >> crypted_data
    >> crypted_key
    >> signature;

  vds_assert(0 == crypted.size());
}

bool vds::transactions::transaction_block::validate_block(
  const certificate & write_cert,
  const guid & channel_id,
  const guid & read_cert_id,
  const guid & write_cert_id,
  const std::set<const_data_buffer> & ancestors,
  const const_data_buffer & crypted_data,
  const const_data_buffer & crypted_key,
  const const_data_buffer & signature) {

  vds_assert(cert_control::get_id(write_cert) == write_cert_id);

  return asymmetric_sign_verify::verify(
    hash::sha256(),
    write_cert.public_key(),
    signature,
    (binary_serializer()
      << channel_id
      << read_cert_id
      << write_cert_id
      << ancestors
      << crypted_key
      << crypted_data).data());
}

void vds::transactions::transaction_block::collect_dependencies(
    vds::database_transaction &t,
    const guid & channel_id,
    std::set<const_data_buffer> &ancestors) const {

  vds::orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.id)
          .where(
              t1.channel_id == channel_id
              && t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while(st.execute()){
    auto id = t1.id.get(st);
    if(ancestors.end() == ancestors.find(vds::base64::to_bytes(id))) {
      ancestors.emplace(vds::base64::to_bytes(id));
    }
  }
}

vds::const_data_buffer vds::transactions::transaction_block::register_transaction(
	const service_provider & sp,
    vds::database_transaction &t,
    const guid & channel_id,
    const const_data_buffer &block,
    const std::set<const_data_buffer> &ancestors) const {

  auto id = hash::signature(hash::sha256(), block);
  
  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
      t2.id = vds::base64::from_bytes(id),
      t2.channel_id = channel_id,
      t2.data = block,
      t2.state = (uint8_t)orm::transaction_log_record_dbo::state_t::processed));

  for(auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    t.execute(
        t1.update(t1.state = (uint8_t)orm::transaction_log_record_dbo::state_t::processed)
            .where(t1.id == base64::from_bytes(ancestor)));
  }
  
  return id;
}

void vds::transactions::transaction_block::on_new_transaction(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const const_data_buffer & id,
    const const_data_buffer &block) const {

}
