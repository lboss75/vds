#ifndef __VDS_CLIENT_FILE_MANAGER_H_
#define __VDS_CLIENT_FILE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
#include "filename.h"

namespace vds {
  class _file_manager;

  class file_manager
  {
  public:


  private:
    std::shared_ptr<_file_manager> impl_;
  };
}

#endif // __VDS_CLIENT_FILE_MANAGER_H_
