#ifndef UPNP_CLIENT_H
#define UPNP_CLIENT_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "types.h"

namespace vds {
  class upnp_client
  {
  public:
    upnp_client();
    ~upnp_client();
    
    bool open_port(
      const service_provider & sp,
      uint16_t internal_port,
      uint16_t external_port,
      const std::string & protocol,
      const std::string & description
    );
    
    void close_port(
      const service_provider & sp,
      uint16_t external_port,
      const std::string & protocol
    );
    
  private:

#ifdef _WIN32
    com_ptr<IUPnPNAT> nat_;
    com_ptr<IStaticPortMappingCollection> port_mappings_;
#else
    UPNPDev * devlist_;
    UPNPUrls upnp_urls_;
    IGDdatas igd_data_;
    char lanaddr_[64];
#endif//_WIN32
  };
}

#endif // UPNP_CLIENT_H
