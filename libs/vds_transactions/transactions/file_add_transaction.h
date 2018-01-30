#ifndef __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
#define __VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>
#include "transaction_log.h"
#include "binary_serialize.h"
#include "channel_message_transaction.h"
#include "transaction_id.h"

namespace vds {
	namespace transactions {

		class file_add_transaction : public channel_message_transaction {
		public:
			struct file_block_t {
				const_data_buffer block_id;
				const_data_buffer block_key;
				uint32_t block_size;
				std::unordered_map<uint16_t, const_data_buffer> replica_hashes;
			};

			file_add_transaction(
				const guid & channel_id,
				const certificate &read_cert,
				const guid &write_cert_id,
				const asymmetric_private_key &cert_key,
				const std::string &name,
				const std::string &mimetype,
				const std::list<file_block_t> & file_blocks);

			template <typename target>
			static void parse_message(binary_deserializer &data_stream, target t) {
				std::string name;
				std::string mimetype;
				std::list<file_add_transaction::file_block_t> file_blocks;

				data_stream >> name >> mimetype >> file_blocks;

				t(name, mimetype, file_blocks);
			}
		};

		inline file_add_transaction::file_add_transaction(
			const guid & channel_id,
			const certificate &read_cert,
			const guid &write_cert_id,
			const asymmetric_private_key &cert_key,
			const std::string &name,
			const std::string &mimetype,
			const std::list<file_add_transaction::file_block_t> &file_blocks)
			: channel_message_transaction(
				channel_message_id::file_add_transaction,
				channel_id,
				read_cert,
				write_cert_id,
				cert_key,
				(binary_serializer() << name << mimetype << file_blocks).data()) {
		}
	}
}

inline vds::binary_serializer & operator << (
	vds::binary_serializer & s,
	const vds::transactions::file_add_transaction::file_block_t & data) {
	return s << data.block_id << data.block_key << data.block_size;
}

inline vds::binary_deserializer & operator >> (
	vds::binary_deserializer & s,
	vds::transactions::file_add_transaction::file_block_t & data) {
	return s >> data.block_id >> data.block_key >> data.block_size;
}

#endif //__VDS_FILE_MANAGER_FILE_ADD_TRANSACTION_H_
