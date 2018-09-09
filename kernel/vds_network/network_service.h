#ifndef __VDS_NETWORK_NETWORK_SERVICE_H_
#define __VDS_NETWORK_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {
    class _network_service;

    class inetwork_service {
    public:

    };

    class network_service : public iservice_factory
    {
    public:
      network_service();
      ~network_service();

      // Inherited via iservice
      void register_services(service_registrator &) override;
      void start(const service_provider &) override;
      void stop(const service_provider &) override;
      std::future<void> prepare_to_stop(const service_provider &) override;
      
      static std::string to_string(const struct sockaddr & from, size_t from_len);
      static std::string to_string(const struct sockaddr_in & from);
      static std::string get_ip_address_string(const sockaddr_in & from);
        
    private:
      _network_service * const impl_;
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_H_
