#ifndef __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
#define __VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "file_operations.h"

namespace vds {
  namespace file_manager_private {
    class _file_operations : public std::enable_shared_from_this<_file_operations> {
    public:
      async_task<> upload_file(
          const service_provider &sp,
          const guid & channel_id,
          const std::string &name,
          const std::string &mimetype,
          const filename &file_path);

    private:
      void pack_file(
          const vds::service_provider &sp,
          const vds::filename &file_path,
          vds::database_transaction &t,
          std::list<std::string> &file_blocks) const;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILE_OPERATIONS_P_H_
