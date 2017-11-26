#ifndef __VDS_USER_MANAGER_CERT_CONTROL_H_
#define __VDS_USER_MANAGER_CERT_CONTROL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class cert_control {
  public:
    static class guid get_id(
        const class certificate & cert);

  private:

  };
}

#endif //__VDS_USER_MANAGER_CERT_CONTROL_H_
