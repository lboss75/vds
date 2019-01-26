#ifndef __VDS_GET_ROOT_GET_ROOT_APP_H__
#define __VDS_GET_ROOT_GET_ROOT_APP_H__

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "command_line.h"
#include "task_manager.h"
#include "mt_service.h"
#include "crypto_service.h"
#include "app.h"

namespace vds {
  class command_line;
  class service_registrator;
  class service_provider;

  class get_root_app : public console_app<get_root_app>
  {
    using base_class = console_app<get_root_app>;
  public:
    get_root_app();

    expected<void> main(const service_provider * sp);

    void register_services(service_registrator & registrator);
    void register_command_line(command_line & cmd_line);
    expected<void> start_services(service_registrator & registrator, service_provider * sp);

  private:
    command_line_set key_generate_command_set_;

    command_line_value user_login_;
    command_line_value user_password_;


    task_manager task_manager_;
    mt_service mt_service_;
    crypto_service crypto_service_;
  };
}

#endif//__VDS_GET_ROOT_GET_ROOT_APP_H__