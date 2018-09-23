#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_statistic.h"

namespace vds {
  class server : public iservice_factory
  {
  public:
    server();
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    vds::async_task<void> prepare_to_stop(const service_provider &sp) override;


    vds::async_task<void> start_network(const vds::service_provider &sp, uint16_t port);

    operator bool () const {
      return nullptr != this->impl_;
    }

    class _server *operator -> () const {
      return this->impl_.get();
    }

    vds::async_task<server_statistic> get_statistic(const service_provider &sp) const;

  private:
    std::shared_ptr<_server> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_H_
