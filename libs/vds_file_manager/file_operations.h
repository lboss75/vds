#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <string>
#include "user_message_transaction.h"
#include "const_data_buffer.h"
#include "async_task.h"
#include "async_buffer.h"

namespace vds {
  class user_manager;

  namespace file_manager_private {
    class _file_operations;
  };

  namespace file_manager {
    class file_operations {
    public:
      struct download_result_t {
        std::string name;
        std::string mime_type;
        size_t size;
        std::shared_ptr<continuous_buffer<uint8_t>> output_stream;
      };


      file_operations();

			vds::async_task<transactions::user_message_transaction::file_info_t> upload_file(
				const service_provider &sp,
        const std::shared_ptr<user_manager> & user_mng,
        const std::string & name,
        const std::string & mime_type,
        const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream);

      async_task<> create_message(
        const service_provider& sp,
        const std::shared_ptr<user_manager>& user_mng,
        const const_data_buffer& channel_id,
        const std::string& message,
        const std::list<transactions::user_message_transaction::file_info_t>& files);


			vds::async_task<download_result_t> download_file(
		    const service_provider &sp,
        const std::shared_ptr<user_manager> & user_mng,
        const const_data_buffer & channel_id,
        const const_data_buffer & target_file);

    protected:
      std::shared_ptr<file_manager_private::_file_operations> impl_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_H_
