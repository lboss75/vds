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

    static void async(const service_provider & sp, const std::function<void(void)> & handler)
    {
      async_enabled_check(sp);
      sp.get<imt_service>()->do_async(sp, handler);
    }

    static void async(const service_provider & sp, std::function<void(void)> && handler)
    {
      async_enabled_check(sp);
      sp.get<imt_service>()->do_async(sp, std::move(handler));
    }

    static void enable_async(const service_provider & sp);
    static void disable_async(const service_provider & sp);
    static void async_enabled_check(const service_provider & sp);

  private:
    void do_async(const service_provider & sp, const std::function<void(void)> & handler);
    void do_async(const service_provider & sp, std::function<void(void)> && handler);

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
