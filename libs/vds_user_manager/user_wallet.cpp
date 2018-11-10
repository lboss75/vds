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

vds::transactions::transaction_record_state vds::user_wallet::get_balance(database_read_transaction& t) {
    std::set<vds::const_data_buffer> ancestors;

    orm::transaction_log_record_dbo t1;
    auto st = t.get_reader(t1.select(
      t1.id)
      .where(t1.state == orm::transaction_log_record_dbo::state_t::leaf));
    while (st.execute()) {
      ancestors.emplace(t1.id.get(st));
    }

    return transactions::transaction_record_state::load(t, ancestors);
}

vds::transactions::transaction_record_state vds::user_wallet::safe_get_balance(
  const service_provider* sp,
  database_transaction& t) {
  for(;;) {
    try{
      return get_balance(t);
    }
    catch (const transactions::transaction_lack_of_funds & ex) {
      transactions::transaction_log::invalid_block(sp, t, ex.refer_transaction());
    }
    catch (const transactions::transaction_source_not_found_error & ex) {
      transactions::transaction_log::invalid_block(sp, t, ex.refer_transaction());
    }
  }
}

vds::user_wallet vds::user_wallet::create_wallet(
  transactions::transaction_block_builder & log,
  const member_user & target_user,
  const std::string & name)
{
  auto cert_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  asymmetric_public_key cert_public_key(cert_private_key);

  certificate::create_options cert_options;
  cert_options.country = "RU";
  cert_options.organization = "IVySoft";
  cert_options.name = "Wallet " + name;
  cert_options.ca_certificate = target_user.user_certificate().get();
  cert_options.ca_certificate_private_key = target_user.private_key().get();

  auto wallet_cert = certificate::create_new(cert_public_key, cert_private_key, cert_options);

  auto message = std::make_shared<json_object>();

  target_user
  .personal_channel()
  .add_log(
    log, 
    transactions::control_message_transaction::create_wallet_message(
      name,
      wallet_cert,
      cert_private_key));

  return user_wallet(name, std::move(wallet_cert), std::move(cert_private_key));
}

void vds::user_wallet::transfer(
  transactions::transaction_block_builder& log,
  const const_data_buffer & source_transaction,
  const member_user& target_user,
  size_t value) {

  log.add(message_create<transactions::payment_transaction>(
    source_transaction, 
    target_user.user_certificate()->subject(),
    value));
}
