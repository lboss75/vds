#ifndef __VDS_BACKGROUND_BACKGROUND_APP_H_
#define __VDS_BACKGROUND_BACKGROUND_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class background_app : public console_app<background_app>
  {
    using base_class = console_app<background_app>;
  public:
    background_app();

    void main(const service_provider & sp);
    
    void register_services(service_registrator & registrator);
    void register_command_line(command_line & cmd_line);
    void start_services(service_registrator & registrator, service_provider & sp);

  private:
    command_line_set server_start_command_set_;
    command_line_set server_root_cmd_set_;

    command_line_value node_login_;
    command_line_value node_password_;
    command_line_value root_folder_;
    command_line_value port_;
    command_line_switch start_;

    std::list<endpoint> endpoints_;

    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;
    crypto_service crypto_service_;
    server server_;
    connection_manager connection_manager_;
    server_log_sync server_log_sync_;
  };
}

#endif // __VDS_BACKGROUND_BACKGROUND_APP_H_
