#ifndef __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_
#define __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "principal_record.h"

namespace vds {
  class _principal_manager;
  
  class principal_manager
  {
  public:
    principal_manager();
    ~principal_manager();    

    _principal_manager * operator -> () { return this->impl_; }
  private:
    _principal_manager * const impl_;
  };
}

#endif // __VDS_PROTOCOLS_PRINCIPAL_MANAGER_H_
