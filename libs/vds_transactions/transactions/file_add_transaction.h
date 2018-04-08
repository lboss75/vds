#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include "binary_serialize.h"
#include "transaction_id.h"

namespace vds {
	namespace transactions {

		class file_add_transaction {
		public:
      static const uint8_t message_id = (uint8_t)transaction_id::file_add_transaction;

			struct file_block_t {
				const_data_buffer block_id;
				const_data_buffer block_key;
        size_t block_size;
			};

			file_add_transaction(
				const std::string &name,
				const std::string &mimetype,
				const std::list<file_block_t> & file_blocks)
					: name_(name), mimetype_(mimetype), file_blocks_(file_blocks){
			}

      file_add_transaction(binary_deserializer & s){
        s >> this->name_ >> this->mimetype_ >> this->file_blocks_;
      }

      void serialize(binary_serializer & s) const {
        s << this->name_ << this->mimetype_ << this->file_blocks_;
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
			std::string name_;
			std::string mimetype_;
			std::list<file_block_t> file_blocks_;
		};
	}
}

inline vds::binary_serializer & operator << (
	vds::binary_serializer & s,
	const vds::transactions::file_add_transaction::file_block_t & data) {
	s << data.block_id << data.block_key << data.block_size;
  return s;
}

inline vds::binary_deserializer & operator >> (
	vds::binary_deserializer & s,
	vds::transactions::file_add_transaction::file_block_t & data) {
	s >> data.block_id >> data.block_key >> data.block_size;
  return s;
}

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
