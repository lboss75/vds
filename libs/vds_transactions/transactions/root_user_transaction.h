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
      static const uint8_t message_id = (uint8_t)transaction_id::root_user_transaction;

      root_user_transaction(
          const certificate &user_cert,
          const std::string &user_name,
          const const_data_buffer &user_private_key,
          const const_data_buffer &password_hash);

      const certificate &user_cert() const { return this->user_cert_; }

      const std::string &user_name() const { return this->user_name_; }

      const const_data_buffer &user_private_key() const { return this->user_private_key_; }

      const const_data_buffer &password_hash() const { return this->password_hash_; }

      root_user_transaction(class binary_deserializer &b);

      void serialize(class binary_serializer &b) const;

      void apply(
          const service_provider & sp,
          database_transaction & t) const {

        //dbo::user_dbo usr_dbo;
        //t.execute(
        //    usr_dbo.insert_or_ignore(
        //        usr_dbo.id = this->id(),
        //        usr_dbo.login = this->user_name(),
        //        usr_dbo.cert = this->user_cert().der(),
        //        usr_dbo.password_hash = this->password_hash(),
        //        usr_dbo.private_key = this->user_private_key()));

      }

    private:
      certificate user_cert_;
      std::string user_name_;
      const_data_buffer user_private_key_;
      const_data_buffer password_hash_;
    };

    inline root_user_transaction::root_user_transaction(
        const certificate &user_cert,
        const std::string &user_name,
        const const_data_buffer &user_private_key,
        const const_data_buffer &password_hash)
        : user_cert_(user_cert),
          user_name_(user_name),
          user_private_key_(user_private_key),
          password_hash_(password_hash) {
    }

    inline root_user_transaction::root_user_transaction(struct binary_deserializer &b) {
      const_data_buffer cert_der;
      b >> cert_der >> this->user_name_ >> this->user_private_key_ >> this->password_hash_;
      this->user_cert_ = certificate::parse_der(cert_der);
    }

    inline void root_user_transaction::serialize(vds::binary_serializer &b) const {
      b << this->user_cert_.der() << this->user_name_ << this->user_private_key_ << this->password_hash_;
    }
  }
}

#endif //__VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
