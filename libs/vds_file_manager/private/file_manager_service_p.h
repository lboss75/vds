#ifndef __VDS_FILE_MANAGER_FILE_MANAGER_P_H_
#define __VDS_FILE_MANAGER_FILE_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"

#include "file_operations.h"

namespace vds {
  namespace file_manager_private {
    class _file_manager_service {
    public:
      void register_services(service_registrator &);
      void start(const service_provider &);
      void stop(const service_provider &);
      vds::async_task<void> prepare_to_stop(const service_provider &sp);

    private:
      file_manager::file_operations file_operations_;
    };
  }
}

#endif //__VDS_FILE_MANAGER_FILE_MANAGER_P_H_
