#ifndef __VDS_transactions_USER_PROFILE_H_
#define __VDS_USER_MANAGER_USER_PROFILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "const_data_buffer.h"


namespace vds {
  
  class user_profile
  {
  public:
    const_data_buffer password_hash;
    const_data_buffer user_private_key;

    template <typename  visitor_type>
    void visit(visitor_type& v) {
      v(
        password_hash,
        user_private_key
      );
    }
  };
}

#endif // __VDS_USER_MANAGER_USER_PROFILE_H_
