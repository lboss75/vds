#ifndef __VDS_NETWORK_PEER_CHANNEL_H_
#define __VDS_NETWORK_PEER_CHANNEL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
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

    formatter_type get_formatter_type() const;

    void broadcast(const void * data, size_t len);
    void broadcast(const std::string & data);

  private:
    class _peer_channel * const impl_;
  };
}

#endif // __VDS_NETWORK_PEER_CHANNEL_H_
