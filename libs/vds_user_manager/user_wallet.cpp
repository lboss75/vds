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
#include "datacoin_balance_dbo.h"


vds::expected<vds::user_wallet> vds::user_wallet::create_wallet(
  transactions::transaction_block_builder & log,
  const member_user & target_user,
  const std::string & name)
{
  GET_EXPECTED(private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  GET_EXPECTED(public_key, asymmetric_public_key::create(private_key));
  GET_EXPECTED(key_id, public_key.hash(hash::sha256()));
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
  const const_data_buffer & source_wallet,
  const const_data_buffer & target_wallet,
  uint64_t value) {

  GET_EXPECTED(wallet_id, this->public_key().hash(hash::sha256()));

  GET_EXPECTED(data, transactions::payment_transaction::signature_data(issuer, currency, source_transaction, source_wallet, target_wallet, value));
  GET_EXPECTED(signature, asymmetric_sign::signature(hash::sha256(), this->private_key(), data));
  
  return log.add(message_create<transactions::payment_transaction>(
    issuer,
    currency,
    source_transaction,
    source_wallet,
    target_wallet,
    value,
    signature));
}

vds::expected<void> vds::user_wallet::asset_issue(
  transactions::transaction_block_builder & log,
  const std::string & currency,
  uint64_t value,
  const member_user & issuer)
{
  GET_EXPECTED(issuer_id, issuer.user_public_key()->hash(hash::sha256()));
  GET_EXPECTED(wallet_id, this->public_key().hash(hash::sha256()));

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
