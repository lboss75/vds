#ifndef __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_
#define __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "peer_network_schema.h"
#include "peer_channel_p.h"
#include "udp_socket.h"

namespace vds {
  class _peer_network_schema
  {
  public:
    _peer_network_schema(
      const service_provider & sp,
      const std::string & schema);
    virtual ~_peer_network_schema();

    const std::string & schema() const;

    event_source<peer_channel *> & open_channel(const std::string & address);

  protected:
    virtual std::unique_ptr<event_source<peer_channel *>> _open_channel(const std::string & address) = 0;

  private:
    friend class peer_network_schema;

    service_provider sp_;
    std::string schema_;
    peer_network_schema * owner_;

    std::mutex peer_channels_mutex_;
    simple_cache<std::string, std::unique_ptr<event_source<peer_channel *>>> peer_channels_;
  };

  class _udp_network_schema : public _peer_network_schema
  {
  public:
    _udp_network_schema(
      const service_provider & sp
    );
    
  protected:
    std::unique_ptr<event_source<peer_channel *>>  _open_channel(const std::string & address) override;

  private:

    class udp_peer_channel : public _peer_channel
    {
    public:
      udp_peer_channel(
        const service_provider & sp,
        peer_channel::channel_direction direction,
        _udp_network_schema * owner);
      ~udp_peer_channel();

      peer_channel::formatter_type get_formatter_type() const override;
      peer_channel::channel_direction get_channel_direction() const override;

      void broadcast(const const_data_buffer & data) override;
      void broadcast(const std::string & data) override;

    private:
      udp_socket s_;
      _udp_network_schema * owner_;
      peer_channel::channel_direction channel_direction_;
    };

    std::unique_ptr<peer_channel> channel_;
  };
}

#endif // __VDS_NETWORK_PEER_NETWORK_SCHEMA_P_H_
