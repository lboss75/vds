#ifndef __VDS_NETWORK_PEER_NETWORK_H_
#define __VDS_NETWORK_PEER_NETWORK_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "peer_channel.h"
#include "network_serializer.h"

namespace vds {
  class peer_network
  {
  public:
    peer_network(const service_provider & sp);
    ~peer_network();


    template <typename message_type>
    void broadcast(const message_type & message)
    {
      this->for_each_active_channel([&message](peer_channel * channel) {
        switch (channel->get_formatter_type()) {
        case peer_channel::binary:
        {
          network_serializer s;
          message.serialize(s);

          channel->broadcast(s.data().data(), s.data().size());
          break;
        }

        case peer_channel::json:
        {
          channel->broadcast(message.str());
          break;
        }

        default:
          throw new std::logic_error("Invalid login");
        }
      });
    }

  private:
    class _peer_network * impl_;

    void for_each_active_channel(const std::function<void(peer_channel *)> & callback);
  };
}

#endif//__VDS_NETWORK_PEER_NETWORK_H_