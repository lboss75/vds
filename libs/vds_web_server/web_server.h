#ifndef __VDS_WEB_SERVER_WEB_SERVER_H_
#define __VDS_WEB_SERVER_WEB_SERVER_H_
#include "service_provider.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class web_server : public iservice_factory
  {
  public:
    web_server();
    ~web_server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    async_task<> prepare_to_stop(const service_provider &sp) override;

    operator bool () const {
      return nullptr != this->impl_;
    }

    class _web_server * operator -> () const;

  private:
    std::shared_ptr<class _web_server> impl_;
  };
}

#endif // __VDS_WEB_SERVER_WEB_SERVER_H_
