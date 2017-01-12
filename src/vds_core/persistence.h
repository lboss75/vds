#ifndef __VDS_CORE_PERSISTENCE_H_
#define __VDS_CORE_PERSISTENCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "foldername.h"

namespace vds {
  class persistence
  {
  public:
    static foldername current_user();
    static foldername local_machine();
  };
}

#endif // __VDS_CORE_PERSISTENCE_H_
