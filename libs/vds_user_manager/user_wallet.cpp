/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_wallet.h"
#include "member_user.h"
#include "json_object.h"
#include "control_message_transaction.h"
#include "transaction_state_calculator.h"
#include "transaction_lack_of_funds.h"
#include "transaction_source_not_found_error.h"
#include "transaction_log.h"
#include "transaction_log_record_dbo.h"

//vds::expected<vds::transactions::transaction_record_state> vds::user_wallet::get_balance(database_read_transaction& t) {
//    std::set<vds::const_data_buffer> ancestors;
//
//    orm::transaction_log_record_dbo t1;
//  GET_EXPECTED(st, t.get_reader(t1.select(t1.id).where(t1.state == orm::transaction_log_record_dbo::state_t::leaf)));
//  WHILE_EXPECTED(st.execute())
//      ancestors.emplace(t1.id.get(st));
//  WHILE_EXPECTED_END()
//
//    return transactions::transaction_record_state::load(t, ancestors);
//}
//
//vds::expected<vds::transactions::transaction_record_state> vds::user_wallet::safe_get_balance(
//  const service_provider* sp,
//  database_transaction& t) {
//  for(;;) {
//    auto result = get_balance(t);
//    if(result.has_error()) {
//      auto error1 = dynamic_cast<transactions::transaction_lack_of_funds *>(result.error().get());
//      if(error1 != nullptr) {
//        CHECK_EXPECTED(transactions::transaction_log::invalid_block(sp, t, error1->refer_transaction()));
//        continue;
//      }
//
//      auto error2 = dynamic_cast<transactions::transaction_source_not_found_error *>(result.error().get());
//      if (error2 != nullptr) {
//        CHECK_EXPECTED(transactions::transaction_log::invalid_block(sp, t, error2->refer_transaction()));
//        continue;
//      }
//
//      return unexpected(std::move(result.error()));
//    }
//  }
//}

vds::expected<vds::user_wallet> vds::user_wallet::create_wallet(
  transactions::transaction_block_builder & log,
  const member_user & target_user,
  const std::string & name)
{
  GET_EXPECTED(cert_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  GET_EXPECTED(cert_public_key, asymmetric_public_key::create(cert_private_key));

  auto message = std::make_shared<json_object>();

  GET_EXPECTED(pc, target_user.personal_channel());
  CHECK_EXPECTED(pc.add_log(
    log, 
    transactions::control_message_transaction::create_wallet_message(
      name,
      cert_public_key,
      cert_private_key)));

  return user_wallet(name, std::move(cert_public_key), std::move(cert_private_key));
}

vds::expected<void> vds::user_wallet::transfer(
  transactions::transaction_block_builder& log,
  const const_data_buffer & issuer,
  const std::string & currency,
  const const_data_buffer & source_transaction,
  const const_data_buffer & source_user,
  const member_user& target_user,
  uint64_t value) {

  GET_EXPECTED(cert_id, target_user.user_certificate()->hash(hash::sha256()));

  binary_serializer s;
  CHECK_EXPECTED(s << (uint8_t)transactions::payment_transaction::message_id);
  CHECK_EXPECTED(s << issuer);
  CHECK_EXPECTED(s << currency);
  CHECK_EXPECTED(s << source_transaction);
  CHECK_EXPECTED(s << source_user);
  CHECK_EXPECTED(s << cert_id);
  CHECK_EXPECTED(s << value);

  GET_EXPECTED(signature, asymmetric_sign::signature(hash::sha256(), *target_user.private_key(), s.move_data()));


  return log.add(message_create<transactions::payment_transaction>(
    issuer,
    currency,
    source_transaction,
    source_user,
    cert_id,
    value,
    signature));
}
