/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_invitation.h"
#include "private/member_user_p.h"


/*
      case command_id::CertRequest: {
		  std::string login;
		  const_data_buffer password_hash;

		  s >> login >> password_hash;

		  sp.get<logger>()->trace("P2PUDPAPI", sp, "Got CertRequest %s", login.c_str());

		  sp.get<db_model>()->async_transaction(sp, [pthis = this->shared_from_this(), sp, login, password_hash](database_transaction & t){
			  dbo::user_dbo t1;

			  auto st = t.get_reader(
				  t1
				  .select(t1.id, t1.cert, t1.private_key)
				  .where(t1.login == login && t1.password_hash == password_hash));

			  binary_serializer result;
			  if (!st.execute()) {
				  result << (uint8_t)command_id::CertRequestFailed;
			  }
			  else {
				  auto user_id = t1.id.get(st);
				  auto user_cert = certificate::parse_der(t1.cert.get(st));
				  auto private_key = t1.private_key.get(st);

				  std::list<certificate> certificates;
				  do {
					  st = t.get_reader(t1.select(t1.cert, t1.parent).where(t1.id == user_id));
					  if (!st.execute()) {
						  throw std::runtime_error("Database is corrupted");
					  }

					  auto cert = certificate::parse_der(t1.cert.get(st));
					  certificates.push_front(cert);

					  user_id = t1.parent.get(st);
				  } while (user_id);

				  result << (uint8_t)command_id::CertRequestSuccessful;
				  result << safe_cast<uint16_t>(certificates.size());
				  for (auto & cert : certificates) {
					  result << cert.der();
				  }

				  result << private_key;

				  auto user_mng = sp.get<user_manager>();
				  auto common_channel = user_mng->get_common_channel();
				  result
					  << common_channel.id()
					  << common_channel.read_cert().der();

				  result << user_cert.public_key().encrypt(user_mng->get_channel_read_key(common_channel.id()).der(std::string()));
			  }

			  auto this_ = static_cast<_p2p_crypto_tunnel_with_certificate *>(pthis.get());
			  this_->send_crypted_command(sp, const_data_buffer(result.data().data(), result.size()));
		  }).execute([pthis = this->shared_from_this(), sp](const std::shared_ptr<std::exception> & ex){
			  if (ex) {
				  pthis->close(sp, ex);
			  }
		  });

		  break;
	  }

namespace vds {
  class _p2p_crypto_tunnel_with_login : public _p2p_crypto_tunnel {
  public:
    _p2p_crypto_tunnel_with_login(
        const udp_transport::session &session,
        const async_result<> &start_result,
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
        const guid &common_channel_id,
        const certificate &common_channel_read_cert,
        const asymmetric_private_key &common_channel_private_key) {

      sp.get<db_model>()->async_transaction(
              sp,
              [pthis = this->shared_from_this(), sp, private_key, common_channel_id,
                  common_channel_read_cert, common_channel_private_key](
                  database_transaction &t) {

				sp.get<logger>()->info("UDPAPI", sp, "Read certificate %s for channel %s",
					cert_control::get_id(common_channel_read_cert).str().c_str(),
					common_channel_id.str().c_str());

                auto this_ = static_cast<_p2p_crypto_tunnel_with_login *>(pthis.get());
                auto usr_manager = sp.get<user_manager>();

				dbo::user_dbo t1;
                //save certificates
                for (auto &cert : this_->certificate_chain_) {
                  t.execute(
                      t1.insert(
                          t1.id = cert_control::get_user_id(cert),
                          t1.cert = cert.der(),
                          t1.parent = cert_control::get_parent_user_id(cert)
                      ));
                }

                transactions::transaction_block log;
                auto &user_cert = *this_->certificate_chain_.rbegin();
                auto user = usr_manager->import_user(user_cert);

                this_->private_key_ = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
                auto device_user = usr_manager->lock_to_device(sp, t, user, this_->login_, this_->password_,
                                                               private_key,
                                                               this_->device_name_, this_->private_key_,
                                                               common_channel_id, this_->port_,
																common_channel_read_cert, 
																common_channel_private_key);
                this_->certificate_chain_.push_back(device_user.user_certificate());

                sp.get<logger>()->trace(
                    "UDPAPI",
                    sp,
                    "Allow read common channel (%s->%s) by local device user(%s->%s)",
                    common_channel_id.str().c_str(),
                    cert_control::get_id(common_channel_read_cert).str().c_str(),
                    device_user.id().str().c_str(),
                    cert_control::get_id(device_user.user_certificate()).str().c_str());

                log.add(
                    transactions::channel_add_reader_transaction(
                        device_user.id(),
                        device_user.user_certificate(),
                        cert_control::get_id(user_cert),
                        private_key,
                        common_channel_read_cert,
                        common_channel_private_key));

                log.add(
                    transactions::device_user_add_transaction(
                        device_user.id(),
                        device_user.user_certificate()));

                log.save(
                    sp, t,
                    common_channel_read_cert,
                    user_cert,
                    private_key);

                binary_serializer s;
                s << (uint8_t) command_id::CertCain;
                s << safe_cast<uint16_t>(1);
                s << device_user.user_certificate().der();

                this_->session_.send(sp, const_data_buffer(s.data().data(), s.size()));

              })
          .execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception> &ex) {
            auto this_ = static_cast<_p2p_crypto_tunnel_with_login *>(pthis.get());
            if (ex) {
              pthis->close(sp, ex);
              this_->start_result_.error(ex);
            } else {
              this_->start_result_.done();
            }
          });
    }

    void process_input_command(
        const service_provider &sp,
        const command_id command,
        binary_deserializer &s) override {
      switch (command) {
        case command_id::CertCain: {
          _p2p_crypto_tunnel::process_input_command(sp, command, s);

          binary_serializer s;
          s << (uint8_t) command_id::CertRequest;
          s << this->login_ << hash::signature(hash::sha256(), this->password_.c_str(), this->password_.length());

          this->send_crypted_command(sp, const_data_buffer(s.data().data(), s.size()));

          break;
        }
        case command_id::CertRequestSuccessful: {
          sp.get<logger>()->trace("P2PUDPAPI", sp, "CertRequestSuccessful");

          uint16_t cert_count;
          s >> cert_count;

          if (!this->certificate_chain_.empty()) {
            throw std::runtime_error("Invalid logic");
          }

          for (uint16_t i = 0; i < cert_count; ++i) {
            const_data_buffer cert_body;
            s >> cert_body;

            this->certificate_chain_.push_back(certificate::parse_der(cert_body));
          }

          const_data_buffer private_key_data;
          s >> private_key_data;

          auto private_key = asymmetric_private_key::parse_der(
              private_key_data,
              this->password_);

          guid common_channel_id;
          const_data_buffer common_channel_read_cert_data;
          const_data_buffer common_channel_private_key_data;
          s
              >> common_channel_id
              >> common_channel_read_cert_data
              >> common_channel_private_key_data;

          auto common_channel_read_cert = certificate::parse_der(
              common_channel_read_cert_data);
          auto common_channel_private_key = asymmetric_private_key::parse_der(
              private_key.decrypt(common_channel_private_key_data), std::string());

          this->create_device_user(
              sp,
              private_key,
              common_channel_id,
              common_channel_read_cert,
              common_channel_private_key);
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
*/
vds::member_user vds::user_invitation::get_user() const
{
	return member_user(new _member_user(this->user_id_, this->user_certificate_));
}

vds::const_data_buffer vds::user_invitation::pack(const std::string& user_password) const
{
	return  (
		binary_serializer()
		<< this->user_id_
		<< this->user_name_
		<< this->user_certificate_
		<< this->user_private_key_.der(user_password)
		<< this->common_channel_id_
		<< this->common_channel_read_cert_
		<< this->common_channel_private_key_.der(user_password)
		<< this->certificate_chain_
		<< asymmetric_sign::signature(
			hash::sha256(), 
			this->user_private_key_,
			(binary_serializer()
				<< this->user_id_
				<< this->user_name_
				<< this->user_certificate_
				<< this->user_private_key_.der(user_password)
				<< this->common_channel_id_
				<< this->common_channel_read_cert_
				<< this->common_channel_private_key_.der(user_password)
				<< this->certificate_chain_
				<< hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data())).data();
}

vds::user_invitation vds::user_invitation::unpack(const const_data_buffer& data, const std::string& user_password)
{
	guid user_id;
	std::string user_name;
	certificate user_cert;
	const_data_buffer user_key_data;
	guid common_channel_id;
	certificate common_channel_read_cert;
	const_data_buffer common_channel_private_key;

	std::list<certificate> certificate_chain;
	const_data_buffer signature;

	binary_deserializer s(data);
	s
		>> user_id
		>> user_name
		>> user_cert
		>> user_key_data
		>> common_channel_id
		>> common_channel_read_cert
		>> common_channel_private_key
		>> certificate_chain
		>> signature;

	if (!asymmetric_sign_verify::verify(hash::sha256(), user_cert.public_key(), signature,
		(binary_serializer()
			<< user_id
			<< user_name
			<< user_cert
			<< user_key_data
			<< common_channel_id
			<< common_channel_read_cert
			<< common_channel_private_key
			<< certificate_chain
			<< hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data()))
	{
		throw std::runtime_error("Signature error");
	}

	const auto user_private_key = asymmetric_private_key::parse_der(user_key_data, user_password);

	return user_invitation(
		user_id,
		user_name,
		user_cert,
		asymmetric_private_key::parse_der(user_key_data, user_password),
		common_channel_id,
		common_channel_read_cert,
		asymmetric_private_key::parse_der(common_channel_private_key, user_password),
		certificate_chain);
}
