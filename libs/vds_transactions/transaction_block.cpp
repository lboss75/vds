/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "chunk_manager.h"
#include "p2p_network.h"
#include "messages/common_log_record.h"
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

void vds::transactions::transaction_block::save(
    const service_provider &sp,
    database_transaction &t,
    const certificate & common_read_cert,
    const certificate & write_cert,
    const asymmetric_private_key & write_private_key) const {

  vds_assert(0 != this->s_.size());

  std::set<const_data_buffer> ancestors;
  this->collect_dependencies(t, ancestors);

  auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  binary_serializer crypted;
  crypted
      << cert_control::get_id(common_read_cert)
	    << cert_control::get_id(write_cert)
      << ancestors
	    << symmetric_encrypt::encrypt(key, this->s_.data())
      << common_read_cert.public_key().encrypt(key.serialize());

  crypted << asymmetric_sign::signature(
      hash::sha256(),
      write_private_key,
      crypted.data());

  auto id = register_transaction(t, crypted.data(), ancestors);
  on_new_transaction(sp, t, id, crypted.data());
}

void vds::transactions::transaction_block::collect_dependencies(
    vds::database_transaction &t,
    std::set<const_data_buffer> &ancestors) const {

  vds::orm::transaction_log_record_dbo t1;
  auto st = t.get_reader(
      t1.select(t1.id)
          .where(t1.state == (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));
  while(st.execute()){
    auto id = t1.id.get(st);
    if(ancestors.end() == ancestors.find(vds::base64::to_bytes(id))) {
      ancestors.emplace(vds::base64::to_bytes(id));
    }
  }
}

vds::const_data_buffer vds::transactions::transaction_block::register_transaction(
    vds::database_transaction &t,
    const const_data_buffer &block,
    const std::set<const_data_buffer> &ancestors) const {

  auto id = hash::signature(hash::sha256(), block);
  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
      t2.id = vds::base64::from_bytes(id),
      t2.data = block,
      t2.state = (uint8_t)orm::transaction_log_record_dbo::state_t::leaf));

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

  sp.get<p2p_network>()->broadcast(sp, p2p_messages::common_log_record(
      id, block).serialize());

}
