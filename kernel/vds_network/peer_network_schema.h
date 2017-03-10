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
    ~peer_network_schema();

    const std::string & schema() const;
    
    event_source<peer_channel *> & open_channel(const std::string & address);

    static std::unique_ptr<peer_network_schema> udp_schema(const service_provider & sp);

  private:
    peer_network_schema(class _peer_network_schema * impl);
    class _peer_network_schema * const impl_;
  };
}

#endif // __VDS_NETWORK_PEER_NETWORK_SCHEMA_H_
