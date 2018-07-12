#ifndef __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "cert_control.h"
#include "types.h"
#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "binary_serialize.h"
#include "transaction_id.h"
#include "database_orm.h"

namespace vds {
  namespace transactions {
    class root_user_transaction {
    public:
      static const transaction_id message_id = transaction_id::root_user_transaction;

      root_user_transaction(
          const std::string & user_credentials_key,
          const certificate & user_cert,
          const std::string & user_name);

      const std::string & user_credentials_key() const { return this->user_credentials_key_; }

      const certificate &user_cert() const { return this->user_cert_; }

      const std::string &user_name() const { return this->user_name_; }

      root_user_transaction(class binary_deserializer &b);

      void serialize(class binary_serializer &b) const;

    private:
      std::string user_credentials_key_;
      certificate user_cert_;
      std::string user_name_;
    };

    inline root_user_transaction::root_user_transaction(
        const std::string & user_credentials_key,
        const certificate &user_cert,
        const std::string &user_name)
        : user_credentials_key_(user_credentials_key),
          user_cert_(user_cert),
          user_name_(user_name) {
    }

    inline root_user_transaction::root_user_transaction(binary_deserializer &b) {
      const_data_buffer cert_der;
      b >> this->user_credentials_key_ >> cert_der >> this->user_name_;
      this->user_cert_ = certificate::parse_der(cert_der);
    }

    inline void root_user_transaction::serialize(vds::binary_serializer &b) const {
      b << this->user_credentials_key_ << this->user_cert_.der() << this->user_name_;
    }
  }
}

#endif //__VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
