#ifndef __VDS_NODE_VDS_NODE_APP_H_
#define __VDS_NODE_VDS_NODE_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class node_app : public console_app<node_app>
  {
    using base_class = console_app<node_app>;
  public:
    node_app();
    
    void main(const service_provider & sp);
    
    void register_command_line(command_line & cmd_line);
    void register_services(service_registrator & registrator);

    
  private:
    command_line_set add_storage_cmd_set_;
    command_line_set remove_storage_cmd_set_;
    command_line_set list_storage_cmd_set_;
    
    command_line_value storage_path_;

    command_line_set node_install_cmd_set_;

    command_line_value node_login_;
    command_line_value node_password_;

    client client_;

    storage_service storage_service_;
    task_manager task_manager_;
    network_service network_service_;
    crypto_service crypto_service_;
  };
}

#endif // __VDS_NODE_VDS_NODE_APP_H_
