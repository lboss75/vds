#ifndef __VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "database_orm.h"

namespace vds {
  namespace transactions {
    class create_user_transaction {
    public:
      static const transaction_id message_id = transaction_id::create_user_transaction;

      create_user_transaction(
          const std::string & user_credentials_key,
          const certificate &user_cert,
          const std::string &user_name,
          const certificate &parent_cert)
        : user_credentials_key_(user_credentials_key),
          user_cert_(user_cert),
          user_name_(user_name),
          parent_cert_(parent_cert.subject()) {        
      }

      const std::string & user_credentials_key() const { return this->user_credentials_key_; }
      const certificate & user_cert() const { return this->user_cert_; }
      const std::string & user_name() const { return this->user_name_; }
      const std::string & parent_cert() const { return this->parent_cert_; }

      create_user_transaction(
        class binary_deserializer & s) {
        s
          >> this->user_credentials_key_
          >> this->user_cert_
          >> this->user_name_
          >> this->parent_cert_;
      }

      void serialize(class binary_serializer & s) const {
        s
          << this->user_credentials_key_
          << this->user_cert_
          << this->user_name_
          << this->parent_cert_;
      }


    private:
      std::string user_credentials_key_;
      certificate user_cert_;
      std::string user_name_;
      std::string parent_cert_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CREATE_USER_TRANSACTION_H_
