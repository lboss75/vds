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

vds::expected<vds::transactions::transaction_record_state> vds::user_wallet::get_balance(database_read_transaction& t) {
    std::set<vds::const_data_buffer> ancestors;

    orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.id).where(t1.state == orm::transaction_log_record_dbo::state_t::leaf)));
  WHILE_EXPECTED(st.execute())
      ancestors.emplace(t1.id.get(st));
  WHILE_EXPECTED_END()

    return transactions::transaction_record_state::load(t, ancestors);
}

vds::expected<vds::transactions::transaction_record_state> vds::user_wallet::safe_get_balance(
  const service_provider* sp,
  database_transaction& t) {
  for(;;) {
    auto result = get_balance(t);
    if(result.has_error()) {
      auto error1 = dynamic_cast<transactions::transaction_lack_of_funds *>(result.error().get());
      if(error1 != nullptr) {
        CHECK_EXPECTED(transactions::transaction_log::invalid_block(sp, t, error1->refer_transaction()));
        continue;
      }

      auto error2 = dynamic_cast<transactions::transaction_source_not_found_error *>(result.error().get());
      if (error2 != nullptr) {
        CHECK_EXPECTED(transactions::transaction_log::invalid_block(sp, t, error2->refer_transaction()));
        continue;
      }

      return unexpected(std::move(result.error()));
    }
  }
}

vds::expected<vds::user_wallet> vds::user_wallet::create_wallet(
  transactions::transaction_block_builder & log,
  const member_user & target_user,
  const std::string & name)
{
  GET_EXPECTED(cert_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  GET_EXPECTED(cert_public_key, asymmetric_public_key::create(cert_private_key));

  certificate::create_options cert_options;
  cert_options.country = "RU";
  cert_options.organization = "IVySoft";
  cert_options.name = "Wallet " + name;
  cert_options.ca_certificate = target_user.user_certificate().get();
  cert_options.ca_certificate_private_key = target_user.private_key().get();

  GET_EXPECTED(wallet_cert, certificate::create_new(cert_public_key, cert_private_key, cert_options));

  auto message = std::make_shared<json_object>();

  GET_EXPECTED(pc, target_user.personal_channel());
  CHECK_EXPECTED(pc.add_log(
    log, 
    transactions::control_message_transaction::create_wallet_message(
      name,
      wallet_cert,
      cert_private_key)));

  return user_wallet(name, std::move(wallet_cert), std::move(cert_private_key));
}

vds::expected<void> vds::user_wallet::transfer(
  transactions::transaction_block_builder& log,
  const const_data_buffer & source_transaction,
  const member_user& target_user,
  size_t value) {

  return log.add(message_create<transactions::payment_transaction>(
    source_transaction, 
    target_user.user_certificate()->subject(),
    value));
}
