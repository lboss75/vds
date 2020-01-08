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

    expected<void> register_services(service_registrator &) override;
    expected<void> start(const service_provider *) override;
    expected<void> stop() override;
    vds::async_task<vds::expected<void>> prepare_to_stop() override;


    vds::async_task<vds::expected<void>> start_network(
      uint16_t port,
      bool dev_network);

    operator bool () const {
      return nullptr != this->impl_;
    }

    class _server *operator -> () const {
      return this->impl_.get();
    }

    vds::async_task<vds::expected<server_statistic>> get_statistic() const;

  private:
    std::shared_ptr<_server> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_H_
