#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "file_operations.h"
#include "user_message_transaction.h"
#include "async_buffer.h"
#include "hash.h"

namespace vds {
  class user_manager;
}

namespace vds {
  namespace file_manager_private {
    class _file_operations : public std::enable_shared_from_this<_file_operations> {
    public:
			vds::async_task<transactions::user_message_transaction::file_info_t> upload_file(
					
          const std::shared_ptr<user_manager> & user_mng,
          const std::string & name,
          const std::string & mime_type,
					const std::shared_ptr<stream_input_async<uint8_t>> & input_stream);

	    vds::async_task<file_manager::file_operations::download_result_t> download_file(
					
          const std::shared_ptr<user_manager> & user_mng,
          const const_data_buffer & channel_id,
          const const_data_buffer & target_file,
          const std::shared_ptr<stream_output_async<uint8_t>> & output_stream);

      vds::async_task<void> create_message(
        
        const std::shared_ptr<user_manager>& user_mng,
        const const_data_buffer& channel_id,
        const std::string& message,
        const std::list<transactions::user_message_transaction::file_info_t>& files);


      //	    vds::async_task<void> download_block(
//			
//			database_transaction& t,
//      file_manager::download_file_task::block_info & block_id,
//			const std::shared_ptr<file_manager::download_file_task> & result);
      void start(const service_provider * sp);
      void stop();
      vds::async_task<void> prepare_to_stop();

    private:
      const service_provider * sp_;

      struct pack_file_result {
        const_data_buffer total_hash;
        size_t total_size;
        std::list<transactions::user_message_transaction::file_block_t> file_blocks;
      };

      vds::async_task<pack_file_result> pack_file(
          
          const std::shared_ptr<stream_input_async<uint8_t>> & input_stream) const;

//			void restore_chunk(
//					
//					database_transaction& t,
//					file_manager::download_file_task::block_info & block,
//					const std::shared_ptr<file_manager::download_file_task> & result);

      vds::async_task<void> download_stream(
          
          const std::shared_ptr<stream_output_async<uint8_t>> & target_stream,
          const std::list<transactions::user_message_transaction::file_block_t> &file_blocks);
		};
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
