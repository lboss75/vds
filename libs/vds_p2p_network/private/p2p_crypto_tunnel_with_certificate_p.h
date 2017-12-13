#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_CERTIFICATE_P_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_CERTIFICATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <resizable_data_buffer.h>
#include "udp_transport.h"
#include "asymmetriccrypto.h"
#include "symmetriccrypto.h"
#include "binary_serialize.h"
#include "user_dbo.h"
#include "certificate_dbo.h"
#include "cert_control.h"

namespace vds {
  class _p2p_crypto_tunnel_with_certificate : public _p2p_crypto_tunnel {
  public:
    _p2p_crypto_tunnel_with_certificate(
        const udp_transport::session & session,
        const std::list<certificate> & certificate_chain,
        const asymmetric_private_key & private_key)
    : _p2p_crypto_tunnel(session),
      certificate_chain_(certificate_chain),
      private_key_(private_key){
    }

    void start(const service_provider &sp) override;

  private:
    std::list<certificate> certificate_chain_;
    asymmetric_private_key private_key_;

    void process_input_command(
        const service_provider &sp,
        const command_id command,
        binary_deserializer & s) override;
  };

  inline void _p2p_crypto_tunnel_with_certificate::start(const service_provider &sp) {
    _p2p_crypto_tunnel::start(sp);

    binary_serializer s;
    s << (uint8_t)command_id ::CertCain;
    s << safe_cast<uint16_t>(this->certificate_chain_.size());
    for(auto cert : this->certificate_chain_){
      s << cert.der();
    }

    this->session_.send(sp, const_data_buffer(s.data().data(), s.size()));
  }

  inline void _p2p_crypto_tunnel_with_certificate::process_input_command(
      const service_provider &sp,
      const command_id command,
      binary_deserializer & s) {
    switch(command){
      case command_id::SendKey: {
        uint16_t size;
        s >> size;

        resizable_data_buffer crypted_key(size);
        s.pop_data(crypted_key.data(), size);

        auto key_data = this->private_key_.decrypt(crypted_key.data(), size);
        binary_deserializer key_stream(key_data);

        const_data_buffer key_info;
        key_stream >> key_info;

        this->input_key_ = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_info));
        break;
      }
      case command_id::CertRequest:{
        std::string login;
        const_data_buffer password_hash;

        s >> login >> password_hash;

        sp.get<db_model>()->async_transaction(sp, [login, password_hash](database_transaction & t){
          user_dbo t1;

          auto st = t.get_reader(
              t1
                  .select(t1.id, t1.private_key)
                  .where(t1.login == login && t1.password_hash == password_hash));

          binary_serializer result;
          if(!st.execute()){
            result << (uint8_t)command_id::CertRequestFailed;
          }
          else {
            auto id = t1.id.get(st);
            auto private_key = t1.private_key.get(st);

            std::list<certificate> certificates;
            do {
              certificate_dbo t2;
              st = t.get_reader(t2.select(t2.cert).where(t2.id == id));
              if(!st.execute()){
                throw std::runtime_error("Database is corrupted");
              }

              auto cert = certificate::parse_der(t2.cert.get(st));
              certificates.push_front(cert);

              id = cert_control::get_parent_id(cert);
            } while(id);

          }

        });

        break;
      }
      default: {
        _p2p_crypto_tunnel::process_input_command(sp, command, s);
        break;
      }
    }
  }

}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_CERTIFICATE_P_H_
