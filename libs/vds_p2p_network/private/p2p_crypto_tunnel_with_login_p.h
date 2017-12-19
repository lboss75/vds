#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_control.h"
#include "p2p_crypto_tunnel_p.h"
#include "user_manager.h"
#include "chunk_manager.h"
#include "transaction_log.h"
#include "member_user.h"

namespace vds {
  class _p2p_crypto_tunnel_with_login : public _p2p_crypto_tunnel {
  public:
    _p2p_crypto_tunnel_with_login(
        const udp_transport::session &session,
        const std::string &login,
        const std::string &password,
        const std::string &device_name,
        int port)
        : _p2p_crypto_tunnel(session),
          login_(login),
          password_(password),
          device_name_(device_name),
          port_(port) {
    }

  protected:
    void create_device_user(
        const service_provider & sp,
        const std::list<certificate> & certificates,
        const asymmetric_private_key & private_key) {

      sp.get<db_model>()->async_transaction(
              sp,
              [pthis = this->shared_from_this(), sp, certificates, private_key](
          database_transaction & t){
                auto this_ = static_cast<_p2p_crypto_tunnel_with_login *>(pthis.get());
                auto usr_manager = sp.get<user_manager>();

                transaction_block log;
                auto &user_cert = *certificates.rbegin();
                auto user = usr_manager->import_user(user_cert);

                usr_manager->lock_to_device(sp, t, log, user, this_->login_, this_->password_,
                                            private_key, this_->device_name_, this_->port_);
                auto user_id = cert_control::get_id(user_cert);
                auto block_data =  log.sign(
                  user_id,
                  user_cert,
                  user_id,
                  private_key);

                auto block_id = chunk_manager::pack_block(t, block_data);
                transaction_log::apply(sp, t, chunk_manager::get_block(t, block_id));
      })
      .execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception> & ex) {
        if(ex){
          pthis->close(sp, ex);
        }
      });

    }

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

          std::list<certificate> certificates;
          for(uint16_t i = 0; i < cert_count; ++i){
            const_data_buffer cert_body;
            s >> cert_body;

            certificates.push_back(certificate::parse_der(cert_body));
          }

          const_data_buffer private_key_data;
          s >> private_key_data;

          auto private_key = asymmetric_private_key::parse_der(
              private_key_data,
              this->password_);
          this->create_device_user(
              sp,
              certificates,
              private_key);
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
    std::string device_name_;
    int port_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
