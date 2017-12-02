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

namespace vds {
  class _p2p_crypto_tunnel {
  public:

    //Server
    _p2p_crypto_tunnel(
        const udp_transport::session & session,
        const std::list<certificate> & certificate_chain,
        const asymmetric_private_key & private_key)
    : session_(session),
      certificate_chain_(certificate_chain_),
      private_key_(private_key){
    }

    _p2p_crypto_tunnel(
        const udp_transport::session & session)
        : session_(session){
    }

    async_task<> start(const service_provider & sp);

    async_task<> process_input_command(
        const service_provider & sp,
        const const_data_buffer & message);

  private:
    enum class command_id : uint8_t {
      CertCain = 0b00, //size + certificate chain
      SendKey = 0b01, //size + certificate chain
    };

    udp_transport::session session_;

    std::list<certificate> certificate_chain_;
    asymmetric_private_key private_key_;

    symmetric_key output_key_;
    symmetric_key input_key_;


  };

  async_task<> _p2p_crypto_tunnel::start(
      const service_provider &sp) {
    if(this->certificate_chain_.empty()){
      return async_task<>::empty();
    }

    binary_serializer s;
    s << (uint32_t)((uint32_t)command_id::CertCain << 30 | this->certificate_chain_.size());
    for(auto cert : this->certificate_chain_){
      s << cert.der();
    }

    return this->session_.send(sp, const_data_buffer(s.data().data(), s.size()));
  }

  async_task<> _p2p_crypto_tunnel::process_input_command(
      const service_provider &sp,
      const const_data_buffer &message) {
    binary_deserializer s(message);

    uint32_t  header;
    s >> header;
    switch((command_id)(header >> 30)){
      case command_id::CertCain:
      {
        auto size = header & 0x3FFFFFFF;
        std::list<certificate> certificate_chain;
        for(uint32_t i = 0; i < size; ++i){
          const_data_buffer buffer;
          s >> buffer;
          certificate_chain.push_back(certificate::parse_der(buffer));
        }

        //TODO: validate chain

        if(!this->output_key_){
          this->output_key_ = symmetric_key::generate(symmetric_crypto::aes_256_cbc());
        }

        binary_serializer key_stream;
        this->output_key_.serialize(key_stream);

        auto crypted_key = certificate_chain.rbegin()->public_key().encrypt(key_stream.data().data(), key_stream.size());

        binary_serializer out_stream;
        out_stream << (uint32_t)((uint32_t)command_id::SendKey << 30 | crypted_key.size());
        out_stream.push_data(crypted_key.data(), crypted_key.size(), false);

        return this->session_.send(sp, const_data_buffer(out_stream.data().data(), out_stream.size()));
      }
      case command_id::SendKey:
      {
        auto size = header & 0x3FFFFFFF;
        resizable_data_buffer crypted_key(size);
        s.pop_data(crypted_key.data(), size);

        auto key_data = this->private_key_.decrypt(crypted_key.data(), size);
        binary_deserializer key_stream(key_data);

        const_data_buffer key_info;
        key_stream >> key_info;

        this->input_key_ = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_info));

      }
      default:
        throw std::runtime_error("Not implemented");

    }
  }
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_
