#ifndef __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "types.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "const_data_buffer.h"
#include "transaction_log.h"

namespace vds {

  class root_user_transaction {
  public:
    static const uint8_t category_id = transaction_log::user_manager_category_id;
    static const uint8_t message_id = 'c';

    root_user_transaction(
        const guid & id,
        const certificate & user_cert,
        const std::string & user_name,
        const const_data_buffer & user_private_key,
        const const_data_buffer & password_hash);

    const guid & id() const { return this->id_; }
    const certificate & user_cert() const { return this->user_cert_; }
    const std::string & user_name() const { return this->user_name_; }
    const const_data_buffer & user_private_key() const { return this->user_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    root_user_transaction(class binary_deserializer & b);
    void serialize(class binary_serializer & b) const;

  private:
    guid id_;
    certificate user_cert_;
    std::string user_name_;
    const_data_buffer user_private_key_;
    const_data_buffer password_hash_;
  };
}

#endif //__VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
