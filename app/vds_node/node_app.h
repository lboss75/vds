#ifndef __VDS_NODE_VDS_NODE_APP_H_
#define __VDS_NODE_VDS_NODE_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "vds_app.h"

namespace vds {
  class vds_node_app : public vds_console_app<vds_node_app>
  {
  public:
    vds_node_app();
    
    void main(const service_provider & sp);
    
    void register_command_line(vds_command_line & cmd_line);
    
  private:
    vds_command_set add_storage_cmd_set_;
    vds_command_set remove_storage_cmd_set_;
    vds_command_set list_storage_cmd_set_;
    
    vds_command_value storage_path_;
  };
}

#endif // __VDS_NODE_VDS_NODE_APP_H_
