#ifndef __VDS_CORE_MT_SERVICE_H_
#define __VDS_CORE_MT_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "async_task.h"

namespace vds {
  class _mt_service;

  class mt_service : public iservice_factory, public imt_service
  {
  public:
    mt_service();
    ~mt_service();
    
    expected<void> register_services(service_registrator &) override;
    expected<void> start(const service_provider *) override;
    expected<void> stop() override;
    vds::async_task<vds::expected<void>> prepare_to_stop() override;

  private:
    friend class imt_service;
    
    std::unique_ptr<_mt_service> impl_;
  };
  
}

#endif // __VDS_CORE_MT_SERVICE_H_
