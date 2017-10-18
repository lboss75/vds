#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"

namespace vds {
  template <typename item_type>
  class stream
  {
  public:
    virtual ~stream() {}
    
    virtual async_task<> write_async(const item_type * data, size_t len) = 0;
  };
}

#endif//__VDS_CORE_STREAM_H_
