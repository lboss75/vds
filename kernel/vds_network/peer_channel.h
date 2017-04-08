#ifndef __VDS_NETWORK_PEER_CHANNEL_H_
#define __VDS_NETWORK_PEER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _peer_channel;
  
  class peer_channel
  {
  public:
    peer_channel(class _peer_channel * impl);
    ~peer_channel();

    enum formatter_type
    {
      binary,
      json
    };

    formatter_type get_formatter_type() const { return this->formatter_type_; }
    
    enum channel_direction
    {
      client,
      server
    };
    
    channel_direction get_channel_direction() const { return this->channel_direction_; }
    
    void broadcast(const const_data_buffer & data);
    void broadcast(const std::string & data);

  private:
    formatter_type formatter_type_;
    channel_direction channel_direction_;
    _peer_channel * const impl_;
  };
}

#endif // __VDS_NETWORK_PEER_CHANNEL_H_
