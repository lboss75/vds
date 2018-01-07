#ifndef __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
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
    class channel_create_transaction {
    public:
      static const uint8_t message_id = 'c';

      channel_create_transaction(
          const guid &channel_id,
          const guid &owner_id,
          const std::string &name,
          const guid & read_cert_id,
          const guid & write_cert_id)
          : channel_id_(channel_id),
            owner_id_(owner_id),
            name_(name),
            read_cert_id_(read_cert_id),
            write_cert_id_(write_cert_id){
      }



      channel_create_transaction(binary_deserializer & s){
        s
          >> this->channel_id_
          >> this->owner_id_
          >> this->name_
          >> this->read_cert_id_
          >> this->write_cert_id_;
      }

      binary_serializer & serialize(binary_serializer & s) const {
        return s
            << this->channel_id_
            << this->owner_id_
            << this->name_
            << this->read_cert_id_
            << this->write_cert_id_;
      }
    private:
      guid channel_id_;
      guid owner_id_;
      std::string name_;
      guid read_cert_id_;
      guid write_cert_id_;
    };
  }
}

#endif //__VDS_TRANSACTIONS_CHANNEL_CREATE_TRANSACTION_H_
