#ifndef __VDS_BACKGROUND_BACKGROUND_APP_H_
#define __VDS_BACKGROUND_BACKGROUND_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"

namespace vds {
  class vds_cmd_app : public console_app<vds_cmd_app>
  {
    using base_class = console_app<vds_cmd_app>;
  public:
    vds_cmd_app();

    expected<void> main(const service_provider * sp);
    
    void register_services(service_registrator & registrator);
    void register_command_line(command_line & cmd_line);
    expected<void> start_services(service_registrator & registrator, service_provider * sp);
    bool need_demonize();

  private:
    command_line_set server_start_command_set_;
    command_line_set server_root_cmd_set_;
    command_line_set server_init_command_set_;

    command_line_value user_login_;
    command_line_value user_password_;

    command_line_value port_;

    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;
    crypto_service crypto_service_;
    server server_;
  };
}

#endif // __VDS_BACKGROUND_BACKGROUND_APP_H_
