#ifndef __VDS_PEER2PEER_P2P_SERVICE_H_
#define __VDS_PEER2PEER_P2P_SERVICE_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "service_provider.h"

#include "vds_peer2peer.h"

namespace vds {
 
  class p2p_service : public iservice_factory
  {
  public:
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    void prepare_to_stop(const service_provider &) override;
    
    void start_server(uint8_t port);
    void start_client();
    
  private:
      std::shared_ptr<_p2p_service> impl_;
  };
}

#endif // __VDS_PEER2PEER_P2P_SERVICE_H_
