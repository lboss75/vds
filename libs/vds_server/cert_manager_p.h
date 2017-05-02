#ifndef __VDS_SERVER_CERT_MANAGER_P_H_
#define __VDS_SERVER_CERT_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_manager.h"

namespace vds {
  class _cert_manager : public cert_manager
  {
  public:
    bool validate(const certificate & cert);
  };
}

#endif // __VDS_SERVER_CERT_MANAGER_P_H_
