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
      this->s_ << item_type::category_id << item_type::message_id;
      item.serialize(this->s_);
    }

    const_data_buffer sign(
        const class guid & cert_id,
        const class certificate & cert,
        const class asymmetric_private_key & cert_key);

    static const_data_buffer unpack_block(
        const const_data_buffer &data,
        const std::function<void(
            const guid &,
            certificate &,
            asymmetric_private_key &)> & get_cert_handler);
  private:
    binary_serializer s_;
  };
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
