#ifndef __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
#define __VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <set>
#include "binary_serialize.h"

namespace vds {
  namespace transactions {
    class transaction_block {
    public:

      class channel_playback{
      public:
        template<typename item_type>
        void add(
            item_type &&item) {
          this->s_ << item_type::message_id;
          item.serialize(this->s_);
        }

      private:
        binary_serializer s_;
        certificate read_cert_;
        certificate write_cert_;
        asymmetric_private_key write_private_key_;
      };

      channel_playback & create_channel(
          const guid & channel_id,
          const certificate & read_cert,
          const certificate & write_cert,
          const asymmetric_private_key & write_private_key);


      static transaction_block unpack_block(
          const service_provider &sp,
          const const_data_buffer &data,
          const std::function<class certificate(const class guid &)> &get_cert_handler,
          const std::function<class asymmetric_private_key(const class guid &)> &get_key_handler);

    private:
      struct dependency_info {
        const_data_buffer id;
        const_data_buffer key;
      };
      std::map<guid, channel_playback> channels_;
    };
  }
}
#endif //__VDS_TRANSACTIONS_TRANSACTION_BLOCK_H_
