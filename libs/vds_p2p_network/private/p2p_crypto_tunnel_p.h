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

  protected:
    enum class command_id : uint8_t {
      Data = 0,
      CertRequest = 1,//login, hash(password)
      CertRequestFailed = 2,//
      CertCain = 3, //size + certificate chain
      SendKey = 4, //size + certificate chain
    };

    udp_transport::session session_;

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
  };

  inline void _p2p_crypto_tunnel::start(
      const service_provider &sp) {

    this->read_input_messages(sp);
  }

  inline void _p2p_crypto_tunnel::read_input_messages(const service_provider &sp) {
    session_.read_async(sp).execute(
        [pthis = shared_from_this(), sp](
            const std::shared_ptr<std::exception> & ex,
            const const_data_buffer & message){
          if(!ex) {
            auto this_ = static_cast<_p2p_crypto_tunnel *>(pthis.get());
            this_->process_input_command(sp, message);
            this_->read_input_messages(sp);
          }
        });
  }

  inline void _p2p_crypto_tunnel::process_input_command(
      const service_provider &sp,
      const const_data_buffer &message) {
    binary_deserializer s(message);

    uint8_t command;
    s >> command;
    this->process_input_command(sp, (command_id)command, s);
  }

  inline void _p2p_crypto_tunnel::send(
      const service_provider &sp,
      const const_data_buffer &message) {
    if(!this->output_key_){
      throw std::runtime_error("Invalid state");
    }

    this->session_->send(
        sp,
        symmetric_encrypt::encrypt(this->output_key_, message));
  }

  inline async_task<const const_data_buffer &> _p2p_crypto_tunnel::read_async(
      const service_provider &sp) {
    return [pthis = this->shared_from_this()](
        const async_result<const const_data_buffer &> & result){
      static_cast<_p2p_crypto_tunnel *>(pthis.get())->data_read_result_ = result;
    };
  }

  inline void _p2p_crypto_tunnel::process_input_command(
      const service_provider &sp,
      const command_id command,
      binary_deserializer &s) {
    switch(command) {
      case command_id::CertCain: {
        sp.get<logger>()->trace("UDPAPI", sp, "Got CertCain");
        uint16_t size;
        s >> size;

        std::list<certificate> certificate_chain;
        for (decltype(size) i = 0; i < size; ++i) {
          const_data_buffer buffer;
          s >> buffer;
          certificate_chain.push_back(certificate::parse_der(buffer));
        }

        //TODO: validate chain

        if (!this->output_key_) {
          this->output_key_ = symmetric_key::generate(symmetric_crypto::aes_256_cbc());
        } else {
          sp.get<logger>()->warning("UDPAPI", sp, "Duplicate CertCain ");
          return;
        }

        binary_serializer key_stream;
        this->output_key_.serialize(key_stream);

        auto crypted_key = certificate_chain.rbegin()->public_key().encrypt(
            key_stream.data().data(),
            key_stream.size());

        binary_serializer out_stream;
        out_stream << (uint8_t)command_id::SendKey << safe_cast<uint16_t>(crypted_key.size());
        out_stream.push_data(crypted_key.data(), crypted_key.size(), false);

        this->session_.send(sp, const_data_buffer(out_stream.data().data(), out_stream.size()));
        return;
      }
      case command_id::Data: {
        if(!this->input_key_){
          throw std::runtime_error("Invalid state");
        }

        this->data_read_result_.done(
            symmetric_decrypt::decrypt(this->input_key_, s.data(), s.size()));
        break;
      }
      default:
        throw std::runtime_error("Invalid command");
    }
  }

}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_P_H_
