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

    static void async(const service_provider * sp, const std::function<void(void)> & handler)
    {
      sp->get<imt_service>()->do_async(handler);
    }

    static void async(const service_provider * sp, std::function<void(void)> && handler)
    {
      sp->get<imt_service>()->do_async(std::move(handler));
    }

  private:
    void do_async(const std::function<void(void)> & handler);
    void do_async(std::function<void(void)> && handler);

  };


  class mt_service : public iservice_factory, public imt_service
  {
  public:
    mt_service();
    ~mt_service();
    
    void register_services(service_registrator &) override;
    void start(const service_provider *) override;
    void stop() override;
    vds::async_task<void> prepare_to_stop() override;

  private:
    friend class imt_service;
    
    std::unique_ptr<_mt_service> impl_;
  };
  
}

#endif // __VDS_CORE_MT_SERVICE_H_
