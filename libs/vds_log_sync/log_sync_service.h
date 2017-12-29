#ifndef __VDS_LOG_SYNC_LOG_SYNC_SERVICE_H_
#define __VDS_LOG_SYNC_LOG_SYNC_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {

  class log_sync_service {
  public:
    log_sync_service();
    ~log_sync_service();

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

  private:
    std::shared_ptr<class _log_sync_service> impl_;
  };
}


#endif //__VDS_LOG_SYNC_LOG_SYNC_SERVICE_H_