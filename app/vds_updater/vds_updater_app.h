#ifndef __VDS_UPDATER_VDS_UPDATER_APP_H_
#define __VDS_UPDATER_VDS_UPDATER_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"

namespace vds {
namespace updater {
  class vds_updater_app : public console_app
  {
    using base_class = console_app;
  public:
    vds_updater_app();

    expected<void> main(const service_provider * sp) override;
    
    void register_command_line(command_line & cmd_line) override;

  private:
    command_line_set update_command_set_;
  };
}
}

#endif // __VDS_UPDATER_VDS_UPDATER_APP_H_
