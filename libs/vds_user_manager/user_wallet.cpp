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
