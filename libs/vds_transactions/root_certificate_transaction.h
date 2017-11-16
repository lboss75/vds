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

namespace vds {

  class root_certificate_transaction {
  public:
    static const uint8_t message_id = 'c';
    static const char message_type[];

    root_certificate_transaction(
        const guid & id,
        const certificate & user_cert,
        const const_data_buffer & user_private_key,
        const const_data_buffer & password_hash);

    root_certificate_transaction(const std::shared_ptr<class json_value> & source);

    const guid & id() const { return this->id_; }
    const certificate & user_cert() const { return this->user_cert_; }
    const const_data_buffer & user_private_key() const { return this->user_private_key_; }
    const const_data_buffer & password_hash() const { return this->password_hash_; }

    std::shared_ptr<json_value> serialize(bool add_type) const;

    root_certificate_transaction(class binary_deserializer & b);

    void serialize(class binary_serializer & b) const;

  private:
    guid id_;
    certificate user_cert_;
    const_data_buffer user_private_key_;
    const_data_buffer password_hash_;
  };
}

#endif //__VDS_TRANSACTIONS__ROOT_CERTIFICATE_TRANSACTION_H_
