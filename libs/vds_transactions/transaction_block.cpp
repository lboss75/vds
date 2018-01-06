/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include <chunk_manager.h>
#include <transaction_log_record_relation_dbo.h>
#include <transaction_log_dependency_dbo.h>
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

void vds::transactions::transaction_block::save(
    database_transaction & t) const {

  binary_serializer s;
  std::set<std::string> ancestors;

  this->collect_dependencies(s, t, ancestors);

  auto block = chunk_manager::save_block(t, s.data());

  register_transaction(t, ancestors, block);

}

struct dependency_info {
  vds::const_data_buffer id;
  vds::const_data_buffer key;
};

static vds::binary_serializer &operator << (
    vds::binary_serializer & s,
    const dependency_info & info){
  s << info.id << info.key;
  return  s;
}

void vds::transactions::transaction_block::collect_dependencies(
    vds::binary_serializer &s,
    vds::database_transaction &t,
    std::set<std::string> &ancestors) const {
  for(auto & p : this->channels_){
    std::list<dependency_info> dependencies;
    vds::orm::transaction_log_dependency_dbo t1;
    auto st = t.get_reader(
        t1.select(t1.block_id, t1.block_key)
            .where(t1.channel_id == p.first));
    while(st.execute()){
      auto id = t1.block_id.get(st);
      if(ancestors.end() == ancestors.find(vds::base64::from_bytes(id))) {
        ancestors.emplace(vds::base64::from_bytes(id));
      }
      dependencies.push_back(dependency_info {
          .id = id,
          .key = t1.block_key.get(st)
      });
    }

    s << dependencies << p.first << p.second.data();
  }
}

void vds::transactions::transaction_block::register_transaction(
    vds::database_transaction &t,
    const std::set<std::string> &ancestors,
    const vds::chunk_manager::chunk_info &block) const {

  orm::transaction_log_record_dbo t2;
  t.execute(t2.insert(
      t2.id = vds::base64::from_bytes(block.id),
      t2.key = block.key,
      t2.state = (uint8_t) orm::transaction_log_record_dbo::state_t::leaf));

  for(auto & ancestor : ancestors){
    orm::transaction_log_record_dbo t1;
    t.execute(
        t1.update(
            t1.id = ancestor,
            t1.state = (uint8_t)orm::transaction_log_record_dbo::state_t::processed));

    orm::transaction_log_record_relation_dbo t2;
    t.execute(t2.insert(
        t2.predecessor_id = ancestor,
        t2.follower_id = vds::base64::from_bytes(block.id),
        t2.relation_type = (uint8_t) orm::transaction_log_record_relation_dbo::relation_type_t::week
    ));
  }

  for(auto & p : this->channels_){
    orm::transaction_log_dependency_dbo t1;
    t.execute(t1.delete_if(t1.channel_id == p.first));
    t.execute(t1.insert(
        t1.channel_id = p.first,
        t1.block_id = block.id,
        t1.block_key = block.key));
  }
}
