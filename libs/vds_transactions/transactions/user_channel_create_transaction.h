#ifndef __VDS_TRANSACTIONS_USER_CHANNEL_CREATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS_USER_CHANNEL_CREATE_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"
#include "binary_serialize.h"

namespace vds {
  namespace transactions {
    class user_channel_create_transaction {
    public:
      static const uint8_t message_id = 'u';

      user_channel_create_transaction(
          const guid & owner_id,
          const guid & read_cert_id,
          const guid & write_cert_id)
          : owner_id_(owner_id), read_cert_id_(read_cert_id), write_cert_id_(write_cert_id){
      }


      user_channel_create_transaction(binary_deserializer & s){
        s
            >> this->owner_id_
            >> this->read_cert_id_
            >> this->write_cert_id_;
      }

      binary_serializer & serialize(binary_serializer & s) const {
        return s
            << this->owner_id_
            << this->read_cert_id_
            << this->write_cert_id_;
      }
    private:
      guid owner_id_;
      guid read_cert_id_;
      guid write_cert_id_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
