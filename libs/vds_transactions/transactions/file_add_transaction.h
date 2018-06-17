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

		class file_add_transaction {
		public:
      static const channel_message_id message_id = channel_message_id::file_add_transaction;

			struct file_block_t {
				const_data_buffer block_id;
				const_data_buffer block_key;
        std::vector<const_data_buffer> replica_hashes;
        size_t block_size;
			};

			file_add_transaction(
        const const_data_buffer & total_hash,
        size_t total_size,
				const std::string &message,
				const std::string &name,
				const std::string &mimetype,
				const std::list<file_block_t> & file_blocks)
					: total_hash_(total_hash),
						total_size_(total_size),
						message_(message),
						name_(name),
						mimetype_(mimetype),
						file_blocks_(file_blocks){
			}

      file_add_transaction(binary_deserializer & s);
      void serialize(binary_serializer & s) const;

      const const_data_buffer & total_hash() const {
        return this->total_hash_;
      }

      size_t total_size() const {
        return this->total_size_;
      }

			const std::string & message() const {
				return message_;
			}

      const std::string & name() const {
        return name_;
      }

      const std::string & mimetype() const {
        return mimetype_;
      }

      const std::list<file_block_t> & file_blocks() const {
        return file_blocks_;
      }

    private:
      const_data_buffer total_hash_;
      size_t total_size_;
			std::string message_;
      std::string name_;
			std::string mimetype_;
			std::list<file_block_t> file_blocks_;
		};
	}
inline vds::binary_serializer & operator << (
	vds::binary_serializer & s,
	const vds::transactions::file_add_transaction::file_block_t & data) {
	s << data.block_id << data.block_key << data.replica_hashes << data.block_size;
  return s;
}

inline vds::binary_deserializer & operator >> (
	vds::binary_deserializer & s,
	vds::transactions::file_add_transaction::file_block_t & data) {
	s >> data.block_id >> data.block_key >> data.replica_hashes >> data.block_size;
  return s;
}
}


inline vds::transactions::file_add_transaction::file_add_transaction(vds::binary_deserializer & s){
        s
						>> this->total_hash_
						>> this->total_size_
						>> this->message_
						>> this->name_
						>> this->mimetype_
						>> this->file_blocks_;
      }

inline void vds::transactions::file_add_transaction::serialize(vds::binary_serializer & s) const {
        s
						<< this->total_hash_
						<< this->total_size_
						<< this->message_
						<< this->name_
						<< this->mimetype_
						<< this->file_blocks_;
      }

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
