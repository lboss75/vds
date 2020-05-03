#ifndef __VDS_WEB_SERVER_LIB_HTTP_SERVER_H_
#define __VDS_WEB_SERVER_LIB_HTTP_SERVER_H_
#include "service_provider.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class ws_http_server : public iservice_factory
  {
  public:
    ws_http_server();
    ~ws_http_server();

    expected<void> register_services(service_registrator &) override;
    expected<void> start(const service_provider *) override;
    expected<void> stop() override;
    vds::async_task<vds::expected<void>> prepare_to_stop() override;

    operator bool () const {
      return nullptr != this->impl_;
    }

    class _ws_http_server * operator -> () const;

    uint16_t port() const {
      return this->port_;
    }

    void port(uint16_t value) {
      this->port_ = value;
    }

  private:
    uint16_t port_;
    std::shared_ptr<class _ws_http_server> impl_;
  };
}

#endif // __VDS_WEB_SERVER_LIB_HTTP_SERVER_H_
