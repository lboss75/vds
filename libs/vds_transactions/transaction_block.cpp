/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "channel_local_cache_dbo.h"
#include "dht_network_client.h"

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
#include "vds_debug.h"
#include "transaction_log.h"

vds::const_data_buffer vds::transactions::transaction_block::save(
    const service_provider &sp,
    class vds::database_transaction &t,
    const const_data_buffer &channel_id,
    const certificate &read_cert,
    const certificate &write_cert,
    const asymmetric_private_key &write_private_key) const {
    vds_assert(0 != this->data_.size());

    std::set<const_data_buffer> ancestors;
    uint64_t order_no = this->collect_dependencies(t, channel_id, ancestors);

    auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

    binary_serializer crypted;
    crypted
      << (uint8_t)block_type_t::normal
      << channel_id
      << order_no
      << cert_control::get_id(read_cert)
      << cert_control::get_id(write_cert)
      << ancestors
      << symmetric_encrypt::encrypt(key, this->data_.data())
      << read_cert.public_key().encrypt(key.serialize())
      << this->certificates_;

    crypted << asymmetric_sign::signature(
        hash::sha256(),
        write_private_key,
        crypted.data());

    auto id = register_transaction(sp, t, channel_id, crypted.data(), order_no, ancestors);
    on_new_transaction(sp, t, id, crypted.data());

  return id;
}

vds::const_data_buffer vds::transactions::transaction_block::save_self_signed(
  const service_provider &sp,
  class vds::database_transaction &t,
  const const_data_buffer &channel_id,
  const certificate &write_cert,
  const asymmetric_private_key &write_private_key,
  const symmetric_key & user_password_key,
  const const_data_buffer & user_password_hash) const {
  vds_assert(0 != this->data_.size());

  std::set<const_data_buffer> ancestors;
  uint64_t order_no = this->collect_dependencies(t, channel_id, ancestors);
    

  binary_serializer crypted;
  crypted
    << (uint8_t)block_type_t::self_signed
    << channel_id
    << order_no
    << cert_control::get_id(write_cert)
    << cert_control::get_id(write_cert)
    << ancestors
    << symmetric_encrypt::encrypt(user_password_key, this->data_.data())
    << user_password_hash
    << this->certificates_;

  crypted << asymmetric_sign::signature(
    hash::sha256(),
    write_private_key,
    crypted.data());

  auto id = register_transaction(sp, t, channel_id, crypted.data(), order_no, ancestors);
  on_new_transaction(sp, t, id, crypted.data());

  return id;
}

vds::transactions::transaction_block::block_type_t
vds::transactions::transaction_block::parse_block(const const_data_buffer &data, const_data_buffer &channel_id, uint64_t &order_no, guid &read_cert_id,
                                                      guid &write_cert_id, std::set<const_data_buffer> &ancestors, const_data_buffer &crypted_data,
                                                      const_data_buffer &crypted_key,
  std::list<certificate> & certificates,
  const_data_buffer &signature) {

  uint8_t block_type;
  binary_deserializer crypted(data);
  crypted
    >> block_type
    >> channel_id
    >> order_no
    >> read_cert_id
    >> write_cert_id
    >> ancestors
    >> crypted_data
    >> crypted_key
    >> certificates
    >> signature;

  vds_assert(0 == crypted.size());
  return (block_type_t)block_type;
}

bool
vds::transactions::transaction_block::validate_block(const certificate &write_cert, block_type_t block_type, const const_data_buffer &channel_id, uint64_t &order_no, const guid &read_cert_id,
                                                     const guid &write_cert_id, const std::set<const_data_buffer> &ancestors,
                                                     const const_data_buffer &crypted_data, const const_data_buffer &crypted_key,
  const std::list<certificate> & certificates,
  const const_data_buffer &signature) {

  vds_assert(cert_control::get_id(write_cert) == write_cert_id);

  return asymmetric_sign_verify::verify(
    hash::sha256(),
    write_cert.public_key(),
    signature,
    (binary_serializer()
      << (uint8_t)block_type
      << channel_id
      << order_no
      << read_cert_id
      << write_cert_id
      << ancestors
      << crypted_data
      << crypted_key
      << certificates).data());
}

uint64_t vds::transactions::transaction_block::collect_dependencies(
    vds::database_transaction &t,
    const const_data_buffer &channel_id,
    std::set<const_data_buffer> &ancestors) const {

  uint64_t result = 0;
  vds::orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.id, t1.order_no)
          .where(
              t1.channel_id == base64::from_bytes(channel_id)
              && t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while(st.execute()){
    auto id = t1.id.get(st);
    if(ancestors.end() == ancestors.find(vds::base64::to_bytes(id))) {
      ancestors.emplace(vds::base64::to_bytes(id));
    }

    auto order = t1.order_no.get(st);
    if(result < order){
      result = order;
    }
  }

  return result + 1;
}

vds::const_data_buffer vds::transactions::transaction_block::register_transaction(
	const service_provider & sp,
    vds::database_transaction &t,
    const const_data_buffer & channel_id,
    const const_data_buffer &block,
    uint64_t order_no,
    const std::set<const_data_buffer> &ancestors) const {

  auto id = hash::signature(hash::sha256(), block);
  
  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
      t2.id = vds::base64::from_bytes(id),
      t2.channel_id = base64::from_bytes(channel_id),
      t2.data = block,
      t2.state = (uint8_t)orm::transaction_log_record_dbo::state_t::leaf,
      t2.order_no = order_no));

  orm::channel_local_cache_dbo t3;
  t.execute(t3.insert(
      t3.id = vds::base64::from_bytes(id),
      t3.data = block
  ));

  for(auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    t.execute(
        t1.update(t1.state = (uint8_t)orm::transaction_log_record_dbo::state_t::validated)
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
