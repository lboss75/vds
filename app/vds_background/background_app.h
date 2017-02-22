#ifndef __VDS_BACKGROUND_BACKGROUND_APP_H_
#define __VDS_BACKGROUND_BACKGROUND_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class vrt_object;

  class background_app : public console_app<background_app>
  {
    using base_class = console_app<background_app>;
  public:
    background_app();

    void main(const service_provider & sp);
    
    void register_services(service_registrator & registrator);
    void register_command_line(command_line & cmd_line);

  private:
    command_line_set start_server_command_set_;

    network_service network_service_;
    crypto_service crypto_service_;
    task_manager task_manager_;
    storage_service storage_;
    client client_;
    server server_;
  };
}

#endif // __VDS_BACKGROUND_BACKGROUND_APP_H_
