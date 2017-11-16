/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_log.h"
#include "private/transaction_log_p.h"
#include "user_manager.h"
#include "asymmetriccrypto.h"
#include "database_orm.h"
#include "db_model.h"
#include "root_certificate_transaction.h"
#include "transaction_block.h"
#include "member_user.h"

void vds::transaction_log::reset(
    const service_provider &sp,
    database_transaction & t,
    const std::string & root_user_name,
    const std::string & root_password) {

  transaction_block block;

  asymmetric_private_key private_key(asymmetric_crypto::rsa2048());
  private_key.generate();

  auto user = sp.get<user_manager>()->create_root_user(
      t,
      root_user_name,
      root_password,
      private_key);

  block.add(root_certificate_transaction(
    user.id(),
    user.user_certificate(),
    user.private_key(),
    user.password_hash()));


}