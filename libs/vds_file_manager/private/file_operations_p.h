#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "file_operations.h"
#include "transactions/file_add_transaction.h"
#include "async_buffer.h"
#include "hash.h"

namespace vds {
  class user_manager;
}

namespace vds {
  namespace file_manager_private {
    class _file_operations : public std::enable_shared_from_this<_file_operations> {
    public:
      async_task<> upload_file(
          const service_provider &sp,
          const std::shared_ptr<user_manager> & user_mng,
          const const_data_buffer & channel_id,
          const std::string &name,
          const std::string &mimetype,
          const filename &file_path);

			async_task<const_data_buffer> upload_file(
					const service_provider &sp,
          const std::shared_ptr<user_manager> & user_mng,
					const const_data_buffer &channel_id,
					const std::string &name,
					const std::string &mimetype,
					const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream);

	    async_task<> download_file(
					const service_provider &sp,
          const std::shared_ptr<user_manager> & user_mng,
					const std::shared_ptr<file_manager::download_file_task> & task);


	    async_task<> download_block(
			const service_provider& sp,
			database_transaction& t,
      file_manager::download_file_task::block_info & block_id,
			const std::shared_ptr<file_manager::download_file_task> & result);

    private:
      void pack_file(
          const vds::service_provider &sp,
          const vds::filename &file_path,
          vds::database_transaction &t,
          std::list<transactions::file_add_transaction::file_block_t> &file_blocks) const;

      struct pack_file_result {
        const_data_buffer total_hash;
        size_t total_size;
        std::list<transactions::file_add_transaction::file_block_t> file_blocks;
      };

      async_task<pack_file_result> pack_file(
          const service_provider &sp,
          const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream) const;

			void restore_chunk(
					const service_provider& sp,
					database_transaction& t,
					file_manager::download_file_task::block_info & block,
					const std::shared_ptr<file_manager::download_file_task> & result);

		};
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
