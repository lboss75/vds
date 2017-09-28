#ifndef __VDS_CRYPTO_CRYPTO_SERVICE_H_
#define __VDS_CRYPTO_CRYPTO_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {

  class crypto_service : public iservice_factory
  {
  public:
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    static void rand_bytes(void * buffer, size_t buffer_size);

    typedef int certificate_extension_type;
    static certificate_extension_type register_certificate_extension_type(
      const char * oid,
      const char * name,
      const char * description,
      certificate_extension_type base_nid = 0);
  };
}

#endif // __VDS_CRYPTO_CRYPTO_SERVICE_H_
