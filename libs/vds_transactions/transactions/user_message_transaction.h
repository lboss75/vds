#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include "binary_serialize.h"
#include "channel_message_id.h"
#include "json_object.h"
#include "json_parser.h"

namespace vds {
	namespace transactions {

		class user_message_transaction {
		public:
      static const channel_message_id message_id = channel_message_id::user_message_transaction;

			struct file_block_t {
				const_data_buffer block_id;
				const_data_buffer block_key;
        std::vector<const_data_buffer> replica_hashes;
        uint64_t block_size;
			};

      struct file_info_t {
        std::string name;
        std::string mime_type;
        uint64_t size;
        const_data_buffer file_id;
        std::list<file_block_t> file_blocks;
      };

      std::shared_ptr<json_value> message;
      std::list<file_info_t> files;

      template <typename  visitor_type>
      void visit(visitor_type & v) {
        v(
          message,
          files
        );
      }

    };
	}
inline expected<void> serialize (
	vds::binary_serializer & s,
	const vds::transactions::user_message_transaction::file_block_t & data) {
  CHECK_EXPECTED(serialize(s, data.block_id));
  CHECK_EXPECTED(serialize(s, data.block_key));
  CHECK_EXPECTED(serialize(s, data.replica_hashes));
  CHECK_EXPECTED(serialize(s, data.block_size));
  return expected<void>();
}

inline expected<void> deserialize(
	vds::binary_deserializer & s,
	vds::transactions::user_message_transaction::file_block_t & data) {
  CHECK_EXPECTED(deserialize(s, data.block_id));
  CHECK_EXPECTED(deserialize(s, data.block_key));
  CHECK_EXPECTED(deserialize(s, data.replica_hashes));
  CHECK_EXPECTED(deserialize(s, data.block_size));
  return expected<void>();
}

inline expected<void> serialize(
  vds::binary_serializer & s,
  const vds::transactions::user_message_transaction::file_info_t & data) {
  CHECK_EXPECTED(serialize(s, data.name));
  CHECK_EXPECTED(serialize(s, data.mime_type));
  CHECK_EXPECTED(serialize(s, data.size));
  CHECK_EXPECTED(serialize(s, data.file_id));
  CHECK_EXPECTED(serialize(s, data.file_blocks));
  return expected<void>();
}

inline expected<void> deserialize(
  vds::binary_deserializer & s,
  vds::transactions::user_message_transaction::file_info_t & data) {
  CHECK_EXPECTED(deserialize(s, data.name));
  CHECK_EXPECTED(deserialize(s, data.mime_type));
  CHECK_EXPECTED(deserialize(s, data.size));
  CHECK_EXPECTED(deserialize(s, data.file_id));
  CHECK_EXPECTED(deserialize(s, data.file_blocks));
  return expected<void>();
}
}

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
