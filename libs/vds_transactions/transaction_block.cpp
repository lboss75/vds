/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "chunk_manager.h"
#include "transaction_log_record_relation_dbo.h"
#include "transaction_log_dependency_dbo.h"
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

  binary_serializer s;
  std::set<std::string> ancestors;

  this->collect_dependencies(s, t, ancestors);

  vds_assert(0 != s.size());

  auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  binary_serializer crypted;
  crypted
      << cert_control::get_id(common_read_cert)
      << symmetric_encrypt::encrypt(key, s.data())
      << common_read_cert.public_key().encrypt(key.serialize());

  crypted << asymmetric_sign::signature(
      hash::sha256(),
      write_private_key,
      crypted.data());

  auto id = register_transaction(t, ancestors, crypted.data());
  on_new_transaction(sp, t, id, crypted.data());
}

void vds::transactions::transaction_block::collect_dependencies(
    vds::binary_serializer &s,
    vds::database_transaction &t,
    std::set<std::string> &ancestors) const {
  for(auto & p : this->channels_){
    std::list<const_data_buffer> dependencies;
    vds::orm::transaction_log_dependency_dbo t1;
    auto st = t.get_reader(
        t1.select(t1.block_id)
            .where(t1.channel_id == p.first));
    while(st.execute()){
      auto id = t1.block_id.get(st);
      if(ancestors.end() == ancestors.find(vds::base64::from_bytes(id))) {
        ancestors.emplace(vds::base64::from_bytes(id));
      }
      dependencies.push_back(id);
    }

    s << dependencies << p.first << p.second.data();
  }
}

vds::const_data_buffer vds::transactions::transaction_block::register_transaction(
    vds::database_transaction &t,
    const std::set<std::string> &ancestors,
    const const_data_buffer &block) const {

  auto id = hash::signature(hash::sha256(), block);
  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
      t2.id = vds::base64::from_bytes(id),
      t2.data = block,
      t2.state = (uint8_t) orm::transaction_log_record_dbo::state_t::leaf));

  for(auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    t.execute(
        t1.update(t1.state = (uint8_t)orm::transaction_log_record_dbo::state_t::processed)
            .where(t1.id == ancestor));

    orm::transaction_log_record_relation_dbo t2;
    t.execute(t2.insert(
        t2.predecessor_id = ancestor,
        t2.follower_id = vds::base64::from_bytes(id),
        t2.relation_type = (uint8_t) orm::transaction_log_record_relation_dbo::relation_type_t::week
    ));
  }

  for(auto & p : this->channels_){
    orm::transaction_log_dependency_dbo t1;
    t.execute(t1.delete_if(t1.channel_id == p.first));
    t.execute(t1.insert(
        t1.channel_id = p.first,
        t1.block_id = id));
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
