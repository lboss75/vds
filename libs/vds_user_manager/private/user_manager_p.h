#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "user_manager.h"

namespace vds {
  class _user_manager
  {
  public:
    _user_manager(const std::shared_ptr<iuser_manager_storage> & storage);
    ~_user_manager();

  private:
    std::shared_ptr<iuser_manager_storage> storage_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
