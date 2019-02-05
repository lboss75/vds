#ifndef __VDS_WEB_SERVER_WEB_SERVER_APP_H_
#define __VDS_WEB_SERVER_WEB_SERVER_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "web_server.h"
#include "network_service.h"
#include "server.h"

namespace vds {
  class web_server_app : public console_app
  {
    using base_class = console_app;
  public:
    web_server_app();

    expected<void> main(const service_provider * sp) override;
    
    void register_services(service_registrator & registrator) override;
    void register_command_line(command_line & cmd_line) override;

    bool need_demonize() override;

#ifdef _WIN32
    TCHAR * service_name() const override {
      return "ivsoft_vds";
    }
#endif

  private:
    command_line_set server_start_command_set_;
    command_line_set server_service_command_set_;

    command_line_value start_web_;
    command_line_value port_;

    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;
    crypto_service crypto_service_;
    server server_;
    web_server web_server_;
  };
}

#endif // __VDS_WEB_SERVER_WEB_SERVER_APP_H_
