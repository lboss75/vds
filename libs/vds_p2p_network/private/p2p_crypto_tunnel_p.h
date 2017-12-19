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

namespace vds {
  class _p2p_crypto_tunnel : public udp_transport::_session {
  public:

    //Server
    _p2p_crypto_tunnel(
        const udp_transport::session & session)
    : session_(session),
      data_read_result_([](
          const std::shared_ptr<std::exception> & ex,
          const const_data_buffer & message){
        throw std::runtime_error("Invalid logic");
      }){
    }

    virtual void start(const service_provider &sp);

    void send(const service_provider &sp, const const_data_buffer &message) override;

    async_task<const const_data_buffer &> read_async(const service_provider &sp) override;

    void close(const service_provider &sp, const std::shared_ptr<std::exception> &ex) override;

  protected:
    enum class command_id : uint8_t {
      Data = 0,
      CertRequest = 1,//login, hash(password)
      CertRequestFailed = 2,//
      CertRequestSuccessful = 3,//
      CertCain = 4, //size + certificate chain
      SendKey = 5, //size + certificate chain
      Crypted = 6,
      CryptedByInput = 7
    };

    udp_transport::session session_;

    std::mutex key_mutex_;
    symmetric_key output_key_;
    symmetric_key input_key_;

    void process_input_command(
        const service_provider &sp,
        const const_data_buffer &message);

    virtual void process_input_command(
        const service_provider &sp,
        const command_id command,
        binary_deserializer & s);

    async_result<const const_data_buffer &> data_read_result_;

    void read_input_messages(const service_provider &sp);

    guid partner_id_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_
