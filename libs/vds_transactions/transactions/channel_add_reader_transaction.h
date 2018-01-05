#ifndef __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
#define __VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "guid.h"
#include "asymmetriccrypto.h"
#include "transaction_log.h"

namespace vds {
  namespace transactions {
    class channel_add_reader_transaction {
    public:
      static const uint8_t message_id = 'c';

      channel_add_reader_transaction(
          const guid & user_id,
          const certificate & read_cert,
          const asymmetric_private_key & read_private_key);

    };
  }
}


#endif //__VDS_TRANSACTIONS_CHANNEL_ADD_READER_TRANSACTION_H_
