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
      expected<void> register_services(service_registrator &) override;
      expected<void> start(const service_provider *) override;
      expected<void> stop() override;
      vds::async_task<vds::expected<void>> prepare_to_stop() override;
      
      static std::string to_string(const struct sockaddr & from, size_t from_len);
      static std::string to_string(const struct sockaddr_in & from);
      static std::string get_ip_address_string(const sockaddr_in & from);
      
      _network_service * operator -> () const {
        return this->impl_.get();
      }

    private:
      std::shared_ptr<_network_service> impl_;
    };
}

#endif//__VDS_NETWORK_NETWORK_SERVICE_H_
