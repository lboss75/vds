#ifndef __VDS_FILE_MANAGER_FILE_MANAGER_H_
#define __VDS_FILE_MANAGER_FILE_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {

  //Forwards
  namespace file_manager_private {
    class _file_manager_service;
  }

  namespace file_manager {

    class file_manager_service {
    public:
      file_manager_service();
      ~file_manager_service();

      void register_services(service_registrator &);
      void start(const service_provider &);
      void stop(const service_provider &);
      async_task<> prepare_to_stop(const service_provider &sp);

      operator bool() const {
        return nullptr != this->impl_;
      }

    private:
      file_manager_private::_file_manager_service * const impl_;
    };
  }
}


#endif //__VDS_FILE_MANAGER_FILE_MANAGER_H_
