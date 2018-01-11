#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "transaction_log.h"
#include "binary_serialize.h"
#include "channel_message_transaction.h"

namespace vds {
  namespace transactions {

    class file_add_transaction : public channel_message_transaction {
    public:
      static const uint8_t channel_message_id = 'a';

      struct file_block_t {
        const_data_buffer block_id;
        const_data_buffer block_key;
      };

      file_add_transaction(
          const guid & channel_id,
          const certificate &cert,
          const asymmetric_private_key &cert_key,
          const std::string &name,
          const std::string &mimetype,
          const std::list<file_block_t> & file_blocks);

    };

    inline binary_serializer & operator << (
        binary_serializer & s,
        const file_add_transaction::file_block_t & data){
      return s << data.block_id << data.block_key;
    }

    file_add_transaction::file_add_transaction(
        const guid & channel_id,
        const certificate &cert,
        const asymmetric_private_key &cert_key,
        const std::string &name,
        const std::string &mimetype,
        const std::list<file_add_transaction::file_block_t> &file_blocks)
        : channel_message_transaction(
        channel_id,
        cert,
        cert_key,
        (binary_serializer() << name << mimetype << file_blocks).data()) {
    }

  }
}
#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
