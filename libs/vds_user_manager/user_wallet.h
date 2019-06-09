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
    user_wallet() = default;

    user_wallet(
      const std::string & name,
      asymmetric_public_key && cert,
      asymmetric_private_key && private_key)
    : name_(name),
      cert_(std::move(cert)),
      private_key_(std::move(private_key)) {
    }

    //static expected<transactions::transaction_record_state> get_balance(
    //  database_read_transaction& t);

    //static expected<transactions::transaction_record_state> safe_get_balance(
    //  const service_provider * sp,
    //  database_transaction& t);

    static expected<user_wallet> create_wallet(
      transactions::transaction_block_builder & log,
      const member_user & target_user,
      const std::string & name);

    static expected<void> transfer(
      transactions::transaction_block_builder& log,
      const const_data_buffer & issuer,
      const std::string & currency,
      const const_data_buffer & source_transaction,
      const const_data_buffer & source_user,
      const member_user& target_user,
      uint64_t value);

    const std::string& name() const {
      return name_;
    }

    const asymmetric_public_key & cert() const {
      return cert_;
    }

    const asymmetric_private_key& private_key() const {
      return private_key_;
    }

  private:
    std::string name_;
    asymmetric_public_key cert_;
    asymmetric_private_key private_key_;
  };
}

#endif // __VDS_USER_MANAGER_USER_WALLET_H_
