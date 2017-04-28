#ifndef __VDS_PROTOCOLS_SERVER_CERTIFICATE_H_
#define __VDS_PROTOCOLS_SERVER_CERTIFICATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_certificate
  {
  public:
    
    static guid server_id(const certificate & cert);
  };
}

#endif // __VDS_PROTOCOLS_SERVER_CERTIFICATE_H_
