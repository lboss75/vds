#ifndef __VDS_USER_MANAGER_USER_WALLET_H_
#define __VDS_USER_MANAGER_USER_WALLET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <string>
#include "transaction_block.h"
#include "transaction_block_builder.h"
#include "user_channel.h"
#include "transaction_messages_walker.h"
#include "encoding.h"

namespace vds {
  class user_wallet {
  public:
    user_wallet(
      const std::string & name,
      certificate && cert,
      asymmetric_private_key && private_key)
    : name_(name),
      cert_(std::move(cert)),
      private_key_(std::move(private_key)) {
    }

    static user_wallet create_wallet(
      transactions::transaction_block_builder & log,
      const member_user & target_user,
      const std::string & name);

    static void transfer(
      database_read_transaction & t,
      transactions::transaction_block_builder & log,
      const member_user & target_user,
      const member_user & source_user,
      size_t value);

    const std::string& name() const {
      return name_;
    }

    const certificate& cert() const {
      return cert_;
    }

    const asymmetric_private_key& private_key() const {
      return private_key_;
    }

  private:
    std::string name_;
    certificate cert_;
    asymmetric_private_key private_key_;
  };
}

#endif // __VDS_USER_MANAGER_USER_WALLET_H_
