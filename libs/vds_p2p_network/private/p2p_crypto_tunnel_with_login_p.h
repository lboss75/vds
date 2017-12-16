#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "p2p_crypto_tunnel_p.h"

namespace vds {
  class _p2p_crypto_tunnel_with_login : public _p2p_crypto_tunnel {
  public:
    _p2p_crypto_tunnel_with_login(
        const udp_transport::session & session,
        const std::string &login,
        const std::string &password)
        : _p2p_crypto_tunnel(session),
          login_(login),
          password_(password){
    }

  protected:
    void process_input_command(
        const service_provider &sp,
        const command_id command,
        binary_deserializer &s) override {
      switch(command) {
        case command_id::CertCain: {
          _p2p_crypto_tunnel::process_input_command(sp, command, s);

          binary_serializer s;
          s << (uint8_t)command_id::CertRequest;
          s << this->login_ << hash::signature(hash::sha256(), this->password_.c_str(), this->password_.length());

          this->send(sp, const_data_buffer(s.data().data(), s.size()));

          break;
        }
        case command_id::CertRequestSuccessful:{
          uint16_t cert_count;
          s >> cert_count;

          for(uint16_t i = 0; i < cert_count; ++i){
            const_data_buffer cert_body;
            s >> cert_body;

            this->certificates_.push_back(certificate::parse_der(cert_body));
          }

          const_data_buffer private_key_data;
          s >> private_key_data;

          this->private_key_ = asymmetric_private_key::parse_der(
              private_key_data,
              this->password_);
          break;
        }
        default: {
          _p2p_crypto_tunnel::process_input_command(sp, command, s);
          break;
        }
      }
    }

  private:
    std::string login_;
    std::string password_;

    std::list<certificate> certificates_;
    asymmetric_private_key private_key_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
