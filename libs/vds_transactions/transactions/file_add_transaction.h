#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include "transaction_log.h"
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
				uint32_t block_size;
        uint16_t padding;
				std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
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
	s << data.block_id << data.block_key << data.block_size << data.padding;
  size_t count = data.replica_hashes.size();
  s.write_number(count);
  for(auto & p : data.replica_hashes){
    s << p.first << p.second;
  }
  return s;
}

inline vds::binary_deserializer & operator >> (
	vds::binary_deserializer & s,
	vds::transactions::file_add_transaction::file_block_t & data) {
	s >> data.block_id >> data.block_key >> data.block_size >> data.padding;
  auto count = s.read_number();
  while(0 < count--){
    uint16_t replica;
    vds::const_data_buffer replica_data;

    s >> replica >> replica_data;
    data.replica_hashes[replica] = replica_data;
  }

  return s;
}

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
