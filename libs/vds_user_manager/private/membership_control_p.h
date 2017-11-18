#ifndef __VDS_USER_MANAGER_MEMBERSHIP_CONTROL_H_
#define __VDS_USER_MANAGER_MEMBERSHIP_CONTROL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _membership_control {
  public:
    certificate add_member(
        const certificate & current_cert,
        const certificate & member_cert,
        const certificate & admin_cert,
        const asymmetric_private_key & admin_private_key);

  private:


  };

}

#endif //__VDS_USER_MANAGER_MEMBERSHIP_CONTROL_H_
