#ifndef __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_P_H_
#define __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "certificate_authority.h"

namespace vds {
  class _certificate_authority
  {
  public:
    _certificate_authority(certificate_authority* owner);
    ~_certificate_authority();
      
    static certificate create_root_user(
      const guid & id,
      const asymmetric_private_key & private_key);

  private:
      certificate_authority * const owner_;
  };
}

#endif // __VDS_PROTOCOLS_CERTIFICATE_AUTHORITY_P_H_
