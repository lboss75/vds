#ifndef __VDS_NETWORK_NETWORK_SERVICE_H_
#define __VDS_NETWORK_NETWORK_SERVICE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"

namespace vds {
    class _network_service;

    class network_service : public iservice_factory
    {
    public:
      network_service();
      ~network_service();

      // Inherited via iservice
      void register_services(service_registrator &) override;
      void start(const service_provider *) override;
      void stop() override;
      vds::async_task<void> prepare_to_stop() override;
      
      static std::string to_string(const struct sockaddr & from, size_t from_len);
      static std::string to_string(const struct sockaddr_in & from);
      static std::string get_ip_address_string(const sockaddr_in & from);
      
      _network_service * operator -> () const {
        return this->impl_;
      }

    private:
      _network_service * const impl_;
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_H_
