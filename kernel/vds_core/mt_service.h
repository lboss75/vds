#ifndef __VDS_CORE_MT_SERVICE_H_
#define __VDS_CORE_MT_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {
  class _mt_service;

  class imt_service
  {
  public:

    void async(const std::function<void(void)> & handler);

    template <typename class_name>
    void async(void (class_name::*handler)(), class_name * owner)
    {
      this->async(std::bind(handler, owner));
    }

    static void async(const service_provider & sp, const std::function<void(void)> & handler)
    {
      sp.get<imt_service>().async(handler);
    }
  };


  class mt_service : public iservice_factory, public imt_service
  {
  public:
    mt_service();
    ~mt_service();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

  private:
    friend class imt_service;
    
    std::unique_ptr<_mt_service> impl_;
  };
  
}

#endif // __VDS_CORE_MT_SERVICE_H_
