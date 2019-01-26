#ifndef __VDS_CORE_PERSISTENCE_H_
#define __VDS_CORE_PERSISTENCE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "foldername.h"
#include "service_provider.h"

namespace vds {

  class persistence
  {
  public:
    static expected<foldername> current_user(const service_provider * sp);
    static expected<foldername> local_machine(const service_provider * sp);
  };
}

#endif // __VDS_CORE_PERSISTENCE_H_
