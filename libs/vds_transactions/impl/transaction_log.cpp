/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "private/stdafx.h"
#include <set>
#include "include/transaction_log.h"
#include "private/transaction_log_p.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "include/transaction_block_builder.h"
#include "transaction_log_unknown_record_dbo.h"
#include "transactions/channel_add_writer_transaction.h"
#include "transactions/device_user_add_transaction.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/user_channel_create_transaction.h"
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
#include "logger.h"
#include "include/transaction_block_builder.h"
#include "certificate_chain_dbo.h"

void vds::transaction_log::save(
	const service_provider & sp,
	database_transaction & t,
	const const_data_buffer & block_id,
	const const_data_buffer & block_data)
{
	orm::transaction_log_record_dbo t2;
	auto st = t.get_reader(
      t2
          .select(t2.state)
          .where(t2.id == base64::from_bytes(block_id)));

	if (st.execute()) {
		return;//Already exists
	}

  const_data_buffer block_channel_id;
  uint64_t order_no;
  std::string read_cert_id;
  std::string write_cert_id;
  std::set<const_data_buffer> ancestors;
  const_data_buffer crypted_data;
  const_data_buffer crypted_key;
  std::list<certificate> certificates;
  const_data_buffer signature;
  auto block_type = transactions::transaction_block_builder::parse_block(
      block_data,
      block_channel_id,
      order_no,
      read_cert_id,
      write_cert_id,
      ancestors,
      crypted_data,
      crypted_key,
      certificates,
      signature);

  certificate write_cert;
  orm::certificate_chain_dbo t6;
  st = t.get_reader(t6.select(t6.cert).where(t6.id == write_cert_id));
  if(st.execute()) {
    write_cert = certificate::parse_der(t6.cert.get(st));
  } else {
    for (auto & cert : certificates) {
      if (write_cert_id == cert.subject()) {
        write_cert = cert;
        break;
      }
    }
  }

  bool is_validated;
  if(!write_cert){
    is_validated = false;
  }
  else {
    if(!transactions::transaction_block_builder::validate_block(
      write_cert,
      block_type,
      block_channel_id,
      order_no,
      read_cert_id,
      write_cert_id,
      ancestors,
      crypted_data,
      crypted_key,
      certificates,
      signature)){
      sp.get<logger>()->warning(
          ThisModule,
          sp,
          "Invalid signature record %s",
          base64::from_bytes(channel_id).c_str());
      return;
    }
    is_validated = true;
  }

  t.execute(t2.insert(
      t2.id = base64::from_bytes(block_id),
      t2.data = block_data,
      t2.state = is_validated
                 ? (int)(
                    ancestors.empty()
                   ? orm::transaction_log_record_dbo::state_t::leaf
                   : orm::transaction_log_record_dbo::state_t::validated)
                 : (int)orm::transaction_log_record_dbo::state_t::stored,
      t2.order_no = order_no));

  orm::transaction_log_unknown_record_dbo t4;

  for(const auto & ancestor : ancestors) {
    st = t.get_reader(t4.select(t4.id).where(t4.id == base64::from_bytes(ancestor)));
    if(!st.execute()) {
      t.execute(t4.insert(
        t4.id = base64::from_bytes(ancestor),
        t4.channel_id = channel_id,
        t4.follower_id = base64::from_bytes(block_id)
      ));
    }
  }

  std::set<const_data_buffer> followers;
  st = t.get_reader(t4.select(t4.follower_id).where(t4.id == base64::from_bytes(block_id)));
  while(st.execute()){
    followers.emplace(base64::to_bytes(t4.id.get(st)));
  }

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
  }

  for(auto & cert : certificates) {
    t.execute(t6.insert(t6.id = cert.subject(), t6.cert = cert.der(), t6.parent = cert.issuer()));
  }

  if(is_validated) {
    //walk_messages(crypted_data,
    //  []() {
    //  
    //});
  }
}

