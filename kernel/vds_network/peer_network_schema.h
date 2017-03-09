#ifndef __VDS_NETWORK_PEER_NETWORK_SCHEMA_H_
#define __VDS_NETWORK_PEER_NETWORK_SCHEMA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class peer_channel;
  
  class peer_network_schema
  {
  public:
    peer_network_schema();
    
    const std::string & schema() const;
    
    std::unique_ptr<peer_channel> open_channel(const std::string & address);

  private:
    class _peer_network_schema * const impl_;
  };
}

#endif // __VDS_NETWORK_PEER_NETWORK_SCHEMA_H_
