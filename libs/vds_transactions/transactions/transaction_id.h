#ifndef __VDS_TRANSACTIONS__TRANSACTION_ID_H_
#define __VDS_TRANSACTIONS__TRANSACTION_ID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  namespace transactions {
    enum class transaction_id : uint8_t {
      user_channel_create_transaction = 'u',
      root_user_transaction = 'R',
      file_add_transaction = 'a',
      //device_user_add_transaction = 'd',
      create_channel_transaction = 'p',
      channel_message_transaction = 'm',
      channel_create_transaction = 'n',
      channel_add_writer_transaction = 'w',
      channel_add_reader_transaction = 'r',
      channel_remove_reader_transaction = 'e',

      split_block_transaction = 's',


      payment_transaction = 'p',
      channel_message = 'c'
    };
  }
}

#endif //__VDS_TRANSACTIONS__TRANSACTION_ID_H_
