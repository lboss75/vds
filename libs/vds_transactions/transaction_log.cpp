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

void vds::transaction_log::reset(
    const service_provider &sp,
    const database_transaction & t,
    const std::string & root_user_name,
    const std::string & root_password) {

  asymmetric_private_key private_key(asymmetric_crypto::rsa2048());
  private_key.generate();

  sp.get<user_manager>()->create_root_user(t, root_user_name, root_password, private_key);
}