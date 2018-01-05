#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "transaction_block.h"

namespace vds {
  namespace transactions {
    class transaction_block_builder : public transaction_block {
    public:

      void pack(
          const service_provider & sp,
          class database_transaction & t,
          class member_user & owner,
          const asymmetric_private_key & owner_private_key);


    };
  }
}


#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_BUILDER_H_
