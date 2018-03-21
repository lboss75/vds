#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_statistic.h"
#include "leak_detect.h"
#include "device_activation.h"

namespace vds {
  class server : public iservice_factory
  {
  public:
    server();
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    async_task<> prepare_to_stop(const service_provider &sp) override;


    vds::async_task<device_activation> reset(const vds::service_provider &sp, const std::string &root_user_name, const std::string &root_password);

    vds::async_task<> start_network(const vds::service_provider &sp, uint16_t port);

    vds::async_task<> init_server(
		const vds::service_provider &sp,
		const device_activation & request,
		const std::string & user_password,
		const std::string &device_name,
		int port);

    operator bool () const {
      return nullptr != this->impl_;
    }

    class _server *operator -> () const {
      return this->impl_;
    }

    async_task<server_statistic> get_statistic(const service_provider &sp) const;

  private:
    class _server * const impl_;
  };
  
  class iserver
  {
  public:

  };
}

#endif // __VDS_SERVER_SERVER_H_
