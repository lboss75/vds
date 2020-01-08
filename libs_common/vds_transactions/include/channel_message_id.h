#ifndef __VDS_TRANSACTIONS_CHANNEL_MESSAGE_ID_H_
#define __VDS_TRANSACTIONS_CHANNEL_MESSAGE_ID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace transactions {
    enum class channel_message_id : uint8_t {
      user_channel_create_transaction = 'u',
      user_message_transaction = 'a',
      //device_user_add_transaction = 'd',
      //create_channel_transaction = 'p',
      channel_message_transaction = 'm',
      channel_create_transaction = 'n',
      channel_add_writer_transaction = 'w',
      channel_add_reader_transaction = 'r',
      channel_remove_reader_transaction = 'e',

      split_block_transaction = 's',

      control_message_transaction = 'c',
    };
  }
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_ID_H_
