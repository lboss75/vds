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
        const class guid & target_cert_id,
        const class certificate & target_cert,
        const class guid & sign_cert_key_id,
        const class asymmetric_private_key & sign_cert_key);

    static const_data_buffer unpack_block(
        service_provider & sp,
        const const_data_buffer &data,
        const std::function<class certificate(const class guid &)> & get_cert_handler,
        const std::function<class asymmetric_private_key(const class guid &)> & get_key_handler);
  private:
    binary_serializer s_;
  };
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
