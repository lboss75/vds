#ifndef __VDS_HTTP_HTTP_SERVER_H_
#define __VDS_HTTP_HTTP_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "network_manager.h"
#include "http_parser.h"

namespace vds {
  class http_request;
  class http_router;
  
  class http_server : public iservice
  {
  public:
    http_server(http_router * router);

    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    const std::string & address() const {
      return this->address_;
    }

    void address(const std::string & value) {
      this->address_ = value;
    }

    int port() const {
      return this->port_;
    }

    void port(int value) {
      this->port_ = value;
    }

  private:
    std::string address_;
    int port_;
    http_router * router_;
  };
}

#endif // __VDS_HTTP_HTTP_SERVER_H_
