#ifndef __VDS_HTTP_HTTP_CONTEXT_H_
#define __VDS_HTTP_HTTP_CONTEXT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "asymmetriccrypto.h"

namespace vds {
  class http_context : public service_provider::property_holder
  {
  public:
    http_context(const certificate &  peer_certificate);

    const certificate & peer_certificate() const {
      return this->peer_certificate_;
    }
  private:
    certificate peer_certificate_;
  };
}

#endif // __VDS_HTTP_HTTP_CONTEXT_H_
