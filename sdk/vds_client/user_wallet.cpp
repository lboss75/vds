/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_wallet.h"
#include "member_user.h"
#include "json_object.h"
#include "control_message_transaction.h"


vds::expected<vds::user_wallet> vds::user_wallet::create_wallet(
  transactions::transaction_block_builder & log,
  const member_user & target_user,
  const std::string & name)
{
  GET_EXPECTED(private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  GET_EXPECTED(public_key, asymmetric_public_key::create(private_key));
  GET_EXPECTED(key_id, public_key.fingerprint());
  GET_EXPECTED(key_der, public_key.der());

  CHECK_EXPECTED(log.add(message_create<transactions::create_wallet_transaction>(key_id, key_der)));

  auto message = std::make_shared<json_object>();

  GET_EXPECTED(pc, target_user.personal_channel());
  CHECK_EXPECTED(pc.add_log(
    log, 
    transactions::control_message_transaction::create_wallet_message(
      name,
      public_key,
      private_key)));

  return user_wallet(name, std::move(public_key), std::move(private_key));
}

vds::expected<void> vds::user_wallet::transfer(
  transactions::transaction_block_builder& log,
  const const_data_buffer & issuer,
  const std::string & currency,
  const const_data_buffer & source_transaction,
  const const_data_buffer & target_wallet,
  uint64_t value,
  const std::string & payment_type,
  const std::string & notes) {

  GET_EXPECTED(wallet_id, this->public_key().fingerprint());

  GET_EXPECTED(data, transactions::payment_transaction::signature_data(issuer, currency, source_transaction, wallet_id, target_wallet, value, payment_type, notes));
  GET_EXPECTED(signature, asymmetric_sign::signature(hash::sha256(), this->private_key(), data));
  
  return log.add(message_create<transactions::payment_transaction>(
    issuer,
    currency,
    source_transaction,
    wallet_id,
    target_wallet,
    value,
    payment_type,
    notes,
    signature));
}

//vds::expected<uint64_t> vds::user_wallet::transfer(
//  transactions::transaction_block_builder & log,
//  database_read_transaction & t,
//  const const_data_buffer & issuer,
//  const std::string & currency,
//  const const_data_buffer & target_wallet,
//  uint64_t value,
//  const std::string & payment_type,
//  const std::string & notes)
//{
//  GET_EXPECTED(wallet_id, this->public_key().fingerprint());
//
//  uint64_t result = 0;
//
//  orm::datacoin_balance_dbo t1;
//  GET_EXPECTED(st, t.get_reader(t1.select(t1.source_transaction, t1.confirmed_balance).where(t1.owner == wallet_id && t1.issuer == issuer && t1.currency == currency)));
//  WHILE_EXPECTED(st.execute()) {
//    auto v = t1.confirmed_balance.get(st);
//    if (v > value - result) {
//      v = value - result;
//    }
//
//    CHECK_EXPECTED(this->transfer(log, issuer, currency, t1.source_transaction.get(st), target_wallet, v, payment_type, notes));
//    result += v;
//
//    if (result == value) {
//      break;
//    }
//  }
//  WHILE_EXPECTED_END()
//
//  return expected<uint64_t>(result);
//}

vds::expected<void> vds::user_wallet::asset_issue(
  transactions::transaction_block_builder & log,
  const std::string & currency,
  uint64_t value,
  const member_user & issuer)
{
  GET_EXPECTED(issuer_id, issuer.user_public_key()->fingerprint());
  GET_EXPECTED(wallet_id, this->public_key().fingerprint());

  binary_serializer s;
  CHECK_EXPECTED(s << (uint8_t)transactions::asset_issue_transaction::message_id);
  CHECK_EXPECTED(s << issuer_id);
  CHECK_EXPECTED(s << wallet_id);
  CHECK_EXPECTED(s << currency);
  CHECK_EXPECTED(s << value);

  GET_EXPECTED(signature, asymmetric_sign::signature(hash::sha256(), *issuer.private_key(), s.move_data()));

  return log.add(message_create<transactions::asset_issue_transaction>(
    issuer_id,
    wallet_id,
    currency,
    value,
    signature));
}
