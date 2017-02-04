#ifndef __VDS_NODE_VDS_NODE_APP_H_
#define __VDS_NODE_VDS_NODE_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class node_app : public console_app<node_app>
  {
  public:
    node_app();
    
    void main(const service_provider & sp);
    
    void register_command_line(command_line & cmd_line);
    
  private:
    command_line_set add_storage_cmd_set_;
    command_line_set remove_storage_cmd_set_;
    command_line_set list_storage_cmd_set_;
    
    command_line_value storage_path_;
  };
}

#endif // __VDS_NODE_VDS_NODE_APP_H_
