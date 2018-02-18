#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <resizable_data_buffer.h>
#include "udp_transport.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "binary_serialize.h"
#include "logger.h"
#include "guid.h"
#include "private/udp_transport_session_p.h"

namespace vds {
  class _p2p_crypto_tunnel : public _udp_transport_session {
    using base_class = _udp_transport_session;
  public:
	  _p2p_crypto_tunnel(
      const std::shared_ptr<_udp_transport> & owner,
      const network_address &address,
		  const std::list<certificate> & certificate_chain,
		  const asymmetric_private_key & private_key)
		  : base_class(owner, address),
		  certificate_chain_(certificate_chain),
		  private_key_(private_key) {
	  }
    _p2p_crypto_tunnel(
        const std::shared_ptr<_udp_transport> & owner,
        const network_address &address)
        : base_class(owner, address){
    }

    ////Server
    //_p2p_crypto_tunnel(
    //    const udp_transport::session & session)
    //: session_(session) {
    //}

    virtual void start(const service_provider &sp);

    void send(const service_provider &sp, const const_data_buffer &message);

    void close(const service_provider &sp, const std::shared_ptr<std::exception> &ex);

    async_task<> prepare_to_stop(const vds::service_provider &sp);

  protected:
    enum class command_id : uint8_t {
      Data = 0,
      CertCain, //size + certificate chain
      SendKey, //size + certificate chain
      Crypted
    };

    std::mutex key_mutex_;
    symmetric_key output_key_;
    symmetric_key input_key_;

    std::list<certificate> certificate_chain_;
    asymmetric_private_key private_key_;

    guid partner_id_;

    void process_input_command(
        const service_provider &sp,
        const const_data_buffer &message);

    virtual void process_input_command(
        const service_provider &sp,
        const command_id command,
        binary_deserializer & s);

    void send_crypted_command(
        const service_provider &sp,
        const const_data_buffer &message);
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_
