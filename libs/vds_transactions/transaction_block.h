#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "binary_serialize.h"

namespace vds {
  class transaction_block {
  public:

    template <typename item_type>
    void add(item_type && item){
      this->s_ << item_type::message_id;
      item.serialize(this->s_);
    }

  private:
    binary_serializer s_;

  };
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
