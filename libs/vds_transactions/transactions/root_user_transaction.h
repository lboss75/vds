#ifndef __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "cert_control.h"
#include "types.h"
#include "guid.h"
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
          const certificate &user_cert,
          const std::string &user_name);

      const certificate &user_cert() const { return this->user_cert_; }

      const std::string &user_name() const { return this->user_name_; }

      root_user_transaction(class binary_deserializer &b);

      void serialize(class binary_serializer &b) const;

      void apply(
          const service_provider & sp,
          database_transaction & t) const {
      }

    private:
      certificate user_cert_;
      std::string user_name_;
    };

    inline root_user_transaction::root_user_transaction(
        const certificate &user_cert,
        const std::string &user_name)
        : user_cert_(user_cert),
          user_name_(user_name) {
    }

    inline root_user_transaction::root_user_transaction(struct binary_deserializer &b) {
      const_data_buffer cert_der;
      b >> cert_der >> this->user_name_;
      this->user_cert_ = certificate::parse_der(cert_der);
    }

    inline void root_user_transaction::serialize(vds::binary_serializer &b) const {
      b << this->user_cert_.der() << this->user_name_;
    }
  }
}

#endif //__VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
