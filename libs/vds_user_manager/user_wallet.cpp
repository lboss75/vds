/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_wallet.h"
#include "user_wallet_dbo.h"
#include "member_user.h"
#include "json_object.h"
#include "control_message_transaction.h"

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
  database_read_transaction & t,
  transactions::transaction_block_builder& log,
  const member_user& target_user,
  const member_user& source_user, size_t value) {

  orm::user_wallet_dbo t1;
}
