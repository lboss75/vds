#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include "binary_serialize.h"
#include "channel_message_id.h"

namespace vds {
	namespace transactions {

		class user_message_transaction {
		public:
      static const channel_message_id message_id = channel_message_id::user_message_transaction;

			struct file_block_t {
				const_data_buffer block_id;
				const_data_buffer block_key;
        std::vector<const_data_buffer> replica_hashes;
        size_t block_size;
			};

      struct file_info_t {
        std::string name;
        std::string mime_type;
        size_t size;
        const_data_buffer file_id;
        std::list<file_block_t> file_blocks;
      };

      user_message_transaction(
				const std::string &message,
				const std::list<file_info_t> & files,
        const std::string &json_attributes)
					: message_(message),
						files_(files),
            json_attributes_(json_attributes){
			}

      user_message_transaction(binary_deserializer & s);
      void serialize(binary_serializer & s) const;

			const std::string & message() const {
				return message_;
			}

      const std::list<file_info_t> & files() const {
        return files_;
      }

      const std::string & json_attributes() const {
        return json_attributes_;
      }

    private:
			std::string message_;
      std::list<file_info_t> files_;
      std::string json_attributes_;
    };
	}
inline vds::binary_serializer & operator << (
	vds::binary_serializer & s,
	const vds::transactions::user_message_transaction::file_block_t & data) {
	s << data.block_id << data.block_key << data.replica_hashes << data.block_size;
  return s;
}

inline vds::binary_deserializer & operator >> (
	vds::binary_deserializer & s,
	vds::transactions::user_message_transaction::file_block_t & data) {
	s >> data.block_id >> data.block_key >> data.replica_hashes >> data.block_size;
  return s;
}

inline vds::binary_serializer & operator << (
  vds::binary_serializer & s,
  const vds::transactions::user_message_transaction::file_info_t & data) {
  s << data.name << data.mime_type << data.size << data.file_id << data.file_blocks;
  return s;
}

inline vds::binary_deserializer & operator >> (
  vds::binary_deserializer & s,
  vds::transactions::user_message_transaction::file_info_t & data) {
  s >> data.name >> data.mime_type >> data.size >> data.file_id >> data.file_blocks;
  return s;
}
}


inline vds::transactions::user_message_transaction::user_message_transaction(vds::binary_deserializer & s){
        s
						>> this->message_
						>> this->files_
						>> this->json_attributes_;
      }

inline void vds::transactions::user_message_transaction::serialize(vds::binary_serializer & s) const {
        s
						<< this->message_
            << this->files_
            << this->json_attributes_;
}

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
