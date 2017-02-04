#ifndef UPNP_CLIENT_H
#define UPNP_CLIENT_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class upnp_client
  {
  public:
    upnp_client(const service_provider & sp);
    ~upnp_client();
    
    void open_port(uint16_t internal_port, uint16_t external_port);    
    
  private:
    logger log_;

#ifdef _WIN32
#else
    UPNPDev * devlist_;
    UPNPUrls upnp_urls_;
    IGDdatas igd_data_;
    char lanaddr_[64];
#endif//_WIN32
  };
}

#endif // UPNP_CLIENT_H
