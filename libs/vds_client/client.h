#ifndef __VDS_CLIENT_CLIENT_H_
#define __VDS_CLIENT_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client_connection.h"
#include "client_logic.h"

namespace vds {

  class client : public iservice
  {
  public:
    client();
    ~client();

    // Inherited via iservice
    void register_services(service_registrator &) override;
    void start(const service_provider & sp) override;
    void stop(const service_provider & sp) override;


    void connection_closed();
    void connection_error();

    void node_install(const std::string & login, const std::string & password);

  private:
    certificate client_certificate_;
    asymmetric_private_key client_private_key_;
    std::unique_ptr<client_logic> logic_;
  };
}


#endif // __VDS_CLIENT_CLIENT_H_
