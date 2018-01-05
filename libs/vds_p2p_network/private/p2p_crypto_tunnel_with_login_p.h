#ifndef __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
#define __VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <transaction_log_record_dbo.h>
#include "cert_control.h"
#include "p2p_crypto_tunnel_p.h"
#include "user_manager.h"
#include "chunk_manager.h"
#include "transaction_log.h"
#include "member_user.h"
#include "certificate_dbo.h"
#include "chunk_data_dbo.h"

namespace vds {
  class _p2p_crypto_tunnel_with_login : public _p2p_crypto_tunnel {
  public:
    _p2p_crypto_tunnel_with_login(
        const udp_transport::session &session,
        const async_result<> & start_result,
        const std::string &login,
        const std::string &password,
        const std::string &device_name,
        int port)
        : _p2p_crypto_tunnel(session),
          start_result_(start_result),
          login_(login),
          password_(password),
          device_name_(device_name),
          port_(port) {
      this->leak_detect_.name_ = "_p2p_crypto_tunnel_with_login";
    }

  protected:
    void create_device_user(
        const service_provider &sp,
        const asymmetric_private_key &private_key,
        const guid &common_channel_id) {

      sp.get<db_model>()->async_transaction(
              sp,
              [pthis = this->shared_from_this(), sp, private_key, common_channel_id](
          database_transaction & t){
                auto this_ = static_cast<_p2p_crypto_tunnel_with_login *>(pthis.get());
                auto usr_manager = sp.get<user_manager>();

                //save certificates
                for(auto & cert : this_->certificate_chain_){
                  certificate_dbo t1;
                  t.execute(
                    t1.insert(
                        t1.id = cert_control::get_id(cert),
                        t1.cert = cert.der(),
                        t1.parent = cert_control::get_parent_id(cert)
                    ));
                }

                transaction_block log;
                auto &user_cert = *this_->certificate_chain_.rbegin();
                auto user = usr_manager->import_user(user_cert);

                this_->private_key_ = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
                auto device_user = usr_manager->lock_to_device(sp, t, user, this_->login_, this_->password_,
                                                               private_key,
                                                               this_->device_name_, this_->private_key_,
                                                               common_channel_id, this_->port_);
                this_->certificate_chain_.push_back(device_user.user_certificate());

                auto user_id = cert_control::get_id(user_cert);
                log.pack(sp, t, user, private_key);

                //transaction_log::apply(sp, t, chunk_manager::get_block(t, block_id));

                binary_serializer s;
                s << (uint8_t)command_id::CertCain;
                s << safe_cast<uint16_t>(1);
                s << device_user.user_certificate().der();

                this_->session_.send(sp, const_data_buffer(s.data().data(), s.size()));

      })
      .execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception> & ex) {
        auto this_ = static_cast<_p2p_crypto_tunnel_with_login *>(pthis.get());
        if(ex){
          pthis->close(sp, ex);
          this_->start_result_.error(ex);
        }
        else {
          this_->start_result_.done();
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
          sp.get<logger>()->trace("P2PUDPAPI", sp, "CertRequestSuccessful");

          uint16_t cert_count;
          s >> cert_count;

          if(!this->certificate_chain_.empty()){
            throw std::runtime_error("Invalid logic");
          }

          for(uint16_t i = 0; i < cert_count; ++i){
            const_data_buffer cert_body;
            s >> cert_body;

            this->certificate_chain_.push_back(certificate::parse_der(cert_body));
          }

          const_data_buffer private_key_data;
          s >> private_key_data;

          guid common_channel_id;
          s >> common_channel_id;

          auto private_key = asymmetric_private_key::parse_der(
              private_key_data,
              this->password_);
          this->create_device_user(sp, private_key, common_channel_id);
          break;
        }
        default: {
          _p2p_crypto_tunnel::process_input_command(sp, command, s);
          break;
        }
      }
    }

  private:
    async_result<> start_result_;
    std::string login_;
    std::string password_;
    std::string device_name_;
    int port_;
  };
}

#endif //__VDS_P2P_NETWORK_P2P_CRYPTO_TUNNEL_WITH_LOGIN_P_H_
