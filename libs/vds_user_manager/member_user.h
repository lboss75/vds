#ifndef __VDS_USER_MANAGER_MEMBER_USER_H_
#define __VDS_USER_MANAGER_MEMBER_USER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>

namespace vds {
  class _member_user;
  
  class member_user
  {
  public:

  private:
    std::shared_ptr<_member_user> impl_;
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_H_
