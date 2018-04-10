#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <string>
#include <guid.h>
#include <transactions/file_add_transaction.h>
#include "const_data_buffer.h"
#include "async_task.h"
#include "filename.h"
#include "download_file_task.h"
#include "async_buffer.h"

namespace vds {
  class user_manager;

  namespace file_manager_private {
    class _file_operations;
  };

  namespace file_manager {
    class file_operations {
    public:
      static const size_t BLOCK_SIZE = 16 * 1024 * 1024;

      file_operations();

      vds::async_task<> upload_file(
		      const service_provider &sp,
          const std::shared_ptr<user_manager> & user_mng,
		  		const const_data_buffer &channel_id,
          const std::string &name,
		  		const std::string &mimetype,
          const vds::filename &file_path);

			vds::async_task<const_data_buffer> upload_file(
					const service_provider &sp,
          const std::shared_ptr<user_manager> & user_mng,
					const const_data_buffer &channel_id,
					const std::string &name,
					const std::string &mimetype,
					const std::shared_ptr<continuous_buffer<uint8_t>> & input_stream);


			vds::async_task<> download_file(
		  const service_provider &sp,
			const std::shared_ptr<download_file_task> & task);

    protected:
      std::shared_ptr<file_manager_private::_file_operations> impl_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_H_
