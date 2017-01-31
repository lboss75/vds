#ifndef __VDS_CRYPTO_CRYPTO_SERVICE_H_
#define __VDS_CRYPTO_CRYPTO_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {

  class crypto_service : public iservice
  {
  public:
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
  };

}

#endif // __VDS_CRYPTO_CRYPTO_SERVICE_H_
