/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <certificate_unknown_dbo.h>
#include "stdafx.h"
#include "user_manager.h"
#include "member_user.h"
#include "private/user_manager_p.h"
#include "private/member_user_p.h"
#include "database_orm.h"
#include "root_user_transaction.h"
#include "private/cert_control_p.h"
#include "vds_exceptions.h"
#include "channel_create_transaction.h"
#include "channel_add_reader_transaction.h"
#include "channel_add_writer_transaction.h"
#include "db_model.h"
#include "certificate_chain_dbo.h"
#include "dht_object_id.h"
#include "dht_network_client.h"
#include "../vds_dht_network/private/dht_network_client_p.h"
#include "register_request.h"
#include "create_user_transaction.h"
#include "private/user_channel_p.h"

vds::user_manager::user_manager(){
}

vds::user_manager::~user_manager() {
}

vds::user_manager::login_state_t vds::user_manager::get_login_state() const {
  return this->impl_->get_login_state();
}

vds::async_task<> vds::user_manager::update(const service_provider& sp) {
  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this()](database_transaction & t) {
    pthis->impl_->update(sp, t);
    return true;
  });
}

void vds::user_manager::load(
  const service_provider & sp,
  database_transaction & t,
  const std::string &user_credentials_key,
  const asymmetric_private_key & user_private_key)
{
	if (nullptr != this->impl_.get()) {
		throw std::runtime_error("Logic error");
	}

	this->impl_.reset(new _user_manager(
    user_credentials_key,
    user_private_key));

	this->impl_->update(sp, t);
}

vds::async_task<vds::user_channel> vds::user_manager::create_channel(const service_provider& sp,
  const std::string& name) const {
  return this->impl_->create_channel(sp, name);
}

void vds::user_manager::reset(
    const service_provider &sp,
    database_transaction &t,
    const std::string &root_user_name,
    const std::string &root_password,
    const asymmetric_private_key &root_private_key) {

  auto playback = transactions::transaction_block_builder::create_root_block();

  //Create root user
  auto root_user = _member_user::create_root_user(
    sp,
    playback,
    t,
    root_user_name,
    root_password,
    root_private_key);

  playback.save(
      sp,
      t,
      root_user.user_certificate(),
      root_private_key);
}

//vds::async_task<> vds::user_manager::init_server(
//	const service_provider & parent_sp,
//	const std::string &root_user_name,
//	const std::string & user_password,
//	const std::string & device_name,
//	int port)
//{
//	auto sp = parent_sp.create_scope(__FUNCTION__);
//	return sp.get<db_model>()->async_transaction(sp, [this, sp, root_user_name, user_password, device_name, port](database_transaction & t)
//	{
//		this->load(
//				sp,
//				t,
//				dht::dht_object_id::from_user_email(root_user_name),
//				symmetric_key::from_password(user_password),
//				hash::signature(hash::sha256(), user_password.c_str(), user_password.length())
//		);

//    auto user = this->import_user(*request.certificate_chain().rbegin());
//		transactions::transaction_block_builder log;
//
//		auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
//		auto device_user = this->lock_to_device(
//			sp,
//			t,
//			log,
//			request.certificate_chain(),
//			user,
//			request.user_name(),
//			user_password,
//			request.private_key(),
//			device_name,
//			private_key,
//			port);
//
//		log.add(
//			transactions::device_user_add_transaction(
//				device_user.object_id(),
//				device_user.user_certificate()));
//
//		auto blocks = log.save(
//			sp, t,
//          user.object_id(),
//          user.user_certificate(),
//          user.user_certificate(),
//          request.private_key());
		//this->load(sp, t, device_user.object_id());
//
//		return true;
//	});
//}

vds::user_channel vds::user_manager::get_channel(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
  return this->impl_->get_channel(channel_id);
}

std::map<vds::const_data_buffer, vds::user_channel> vds::user_manager::get_channels() const {
  std::list<vds::user_channel> result;

  return this->impl_->channels();
}

bool vds::user_manager::validate_and_save(
		const service_provider & sp,
		const std::list<vds::certificate> &cert_chain) {

  certificate_store store;
  for (const auto & p : cert_chain) {
    auto cert = this->impl_->get_certificate(p.subject());
    if (!cert) {
      cert = p;

      const auto result = store.verify(cert);
      if (0 != result.error_code) {
        sp.get<logger>()->warning(ThisModule, sp, "Invalid certificate %s %s",
          result.error.c_str(),
          result.issuer.c_str());
        return false;
      }
    }

    store.add(cert);
    this->save_certificate(sp, cert);
  }

  return true;
}

void vds::user_manager::save_certificate(
    const vds::service_provider &sp,
    vds::database_transaction &t,
    const vds::certificate &cert) {

  orm::certificate_chain_dbo t1;
  t.execute(
      t1.insert(
          t1.id = cert.subject(),
          t1.cert = cert.der(),
          t1.parent = cert.issuer()));

  orm::certificate_unknown_dbo t2;
  t.execute(t2.delete_if(t2.id == cert.subject()));
}

void vds::user_manager::save_certificate(const vds::service_provider &sp, const vds::certificate &cert) {
  this->impl_->add_certificate(cert);

  sp.get<db_model>()->async_transaction(sp, [cert](database_transaction & t)->bool{

    orm::certificate_chain_dbo t1;
    auto st = t.get_reader(t1.select(t1.id).where(t1.id == cert.subject()));
    if(!st.execute()){
      t.execute(t1.insert(
          t1.id = cert.subject(),
          t1.cert = cert.der(),
          t1.parent = cert.issuer()));
    }

    return true;
  }).execute([sp](const std::shared_ptr<std::exception> & ex){
    if(ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at saving certificate", ex->what());
    }
  });
}

vds::member_user vds::user_manager::get_current_user() const {
  return this->impl_->get_current_user();
}

const vds::asymmetric_private_key& vds::user_manager::get_current_user_private_key() const {
  return this->impl_->get_current_user_private_key();
}

vds::async_task<> vds::user_manager::create_register_request(const service_provider& sp, const std::string& userName,
  const std::string& user_email, const std::string& user_password) {

  return sp.get<db_model>()->async_transaction(
    sp,
    [sp, userName, user_email, user_password](database_transaction & t) {

    auto user_private_key = vds::asymmetric_private_key::generate(
      vds::asymmetric_crypto::rsa4096());

    asymmetric_public_key user_public_key(user_private_key);

    binary_serializer s;
    s
      << userName
      << user_email
      << user_public_key.der()
      << dht::dht_object_id::user_credentials_to_key(user_email, user_password)
      << user_private_key.der(user_password);

    s << asymmetric_sign::signature(hash::sha256(), user_private_key, s.data());

    orm::register_request t1;
    t.execute(
      t1.insert(
        t1.name = userName,
        t1.email = user_email,
        t1.data = s.data(),
        t1.create_time = std::chrono::system_clock::now()));

    return true;
  });
}

bool vds::user_manager::parse_join_request(const vds::service_provider &sp, const vds::const_data_buffer &data,
                                           std::string &userName, std::string &userEmail) {
  return _user_manager::parse_join_request(sp, data, userName, userEmail);
}

vds::async_task<bool> vds::user_manager::approve_join_request(
  const service_provider& sp,
  const const_data_buffer& data) {
  return this->impl_->approve_join_request(sp, data);
}

/////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager(
		const std::string & user_credentials_key,
    const asymmetric_private_key & user_private_key)
		: user_credentials_key_(user_credentials_key),
      user_private_key_(user_private_key),
      login_state_(user_manager::login_state_t::waiting)
{
}

void vds::_user_manager::update(
		const service_provider & parent_scope,
		database_transaction &t) {
	auto sp = parent_scope.create_scope(__FUNCTION__);
	const auto log = sp.get<logger>();
	log->trace(ThisModule, sp, "security_walker::load");

  std::list<const_data_buffer> new_records;
  orm::transaction_log_record_dbo t1;
	auto st = t.get_reader(
			t1.select(t1.id)
					.order_by(t1.order_no));
  while (st.execute()) {
    auto id = t1.id.get(st);
    if (this->processed_.end() != this->processed_.find(id)) {
      continue;
    }

    new_records.push_back(id);
  }

	std::set<const_data_buffer> new_channels;
	for(auto & id : new_records) {
    st = t.get_reader(
      t1.select(t1.data)
      .where(t1.id == id));

    if(!st.execute()) {
      throw std::runtime_error("Invalid program");
    }

    const auto data = t1.data.get(st);
    transactions::transaction_block block(data);
	  block.walk_messages(
      [this, sp](const transactions::root_user_transaction & message)->bool{
        this->root_user_cert_ = message.user_cert();
        this->root_user_name_ = message.user_name();

        if (this->user_credentials_key_ == message.user_credentials_key()) {
          this->user_cert_ = message.user_cert();
          this->user_name_ = message.user_name();
          this->login_state_ = user_manager::login_state_t::login_sucessful;

          auto cp = _user_channel::import_personal_channel(
            sp,
            this->user_cert_,
            this->user_private_key_);
          this->channels_[cp.id()] = cp;

          cp = _user_channel::import_personal_channel(
            sp,
            this->user_cert_,
            this->user_private_key_);
          this->channels_[cp.id()] = cp;
        }

        return true;
      },
      [this, sp](const transactions::create_user_transaction & message)->bool {
        if (this->user_credentials_key_ == message.user_credentials_key()) {
          this->user_cert_ = message.user_cert();
          this->user_name_ = message.user_name();
          this->login_state_ = user_manager::login_state_t::login_sucessful;

          auto cp = _user_channel::import_personal_channel(
            sp,
            this->user_cert_,
            this->user_private_key_);
          this->channels_[cp.id()] = cp;
        }
        return true;
      },
        [this, sp, &new_channels, log](const transactions::channel_message  & message)->bool {
        auto channel = this->get_channel(message.channel_id());
        if (channel) {
          auto channel_read_key = channel->read_cert_private_key(message.channel_read_cert_subject());
          if (channel_read_key) {
            message.walk_messages(channel_read_key,
              [sp, this, channel_id = message.channel_id(), log](const transactions::channel_add_reader_transaction & message)->bool {
              auto cp = user_channel(
                message.id(),
                string2channel_type(message.channel_type()),
                message.name(),
                message.read_cert(),
                message.read_private_key(),
                message.write_cert(),
                asymmetric_private_key());

              this->channels_[cp.id()] = cp;
              log->debug(ThisModule, sp, "Got channel %s reader certificate",
                base64::from_bytes(cp.id()).c_str());

              return true;
            },
              [sp, this, channel_id = message.channel_id(), log](const transactions::channel_add_writer_transaction & message)->bool {
              auto cp = user_channel(
                message.id(),
                string2channel_type(message.channel_type()),
                message.name(),
                message.read_cert(),
                message.read_private_key(),
                message.write_cert(),
                message.write_private_key());

              this->channels_[cp.id()] = cp;

              log->debug(ThisModule, sp, "Got channel %s write certificate",
                base64::from_bytes(cp.id()).c_str());

              return true;
            },
              [sp, this, channel_id = message.channel_id(), log, &new_channels](const transactions::channel_create_transaction & message)->bool {
              if (new_channels.end() == new_channels.find(channel_id)) {
                new_channels.emplace(message.channel_id());
              }
              auto cp = user_channel(
                message.channel_id(),
                string2channel_type(message.channel_type()),
                message.name(),
                message.read_cert(),
                message.read_private_key(),
                message.write_cert(),
                message.write_private_key());

              this->channels_[cp.id()] = cp;

              log->debug(ThisModule, sp, "New channel %s(%s), read certificate %s, write certificate %s",
                base64::from_bytes(message.channel_id()).c_str(),
                message.name().c_str(),
                cp.read_cert().subject().c_str(),
                cp.write_cert().subject().c_str());

              return true;
            });
          }
        }

        return true;
      });
	}
}

void vds::_user_manager::add_certificate(const vds::certificate &cert) {
	this->certificate_chain_[cert.subject()] = cert;
}

vds::member_user vds::_user_manager::get_current_user() const {
  return member_user(this->user_cert_, this->user_private_key_);
}

vds::async_task<bool> vds::_user_manager::approve_join_request(const service_provider& sp, const const_data_buffer& data) {
  try {
    const_data_buffer user_public_key_der;
    std::string user_object_id;
    const_data_buffer user_private_key_der;
    std::string userName;
    std::string userEmail;

    binary_deserializer s(data);
    s
      >> userName
      >> userEmail
      >> user_public_key_der
      >> user_object_id
      >> user_private_key_der;

    const auto pos = s.size();

    const_data_buffer signature;
    s >> signature;

    const auto user_public_key = asymmetric_public_key::parse_der(user_public_key_der);

    if (!asymmetric_sign_verify::verify(
      hash::sha256(),
      user_public_key,
      signature,
      data.data(),
      data.size() - pos)) {
      return vds::async_task<bool>::result(false);
    }
    return sp.get<db_model>()->async_transaction(sp,
      [
        pthis = this->shared_from_this(),
        sp,
        user_public_key,
        user_object_id,
        user_private_key_der,
        userName,
        userEmail
      ](database_transaction & t) {
      certificate::create_options local_user_options;
      local_user_options.country = "RU";
      local_user_options.organization = "IVySoft";
      local_user_options.name = userName;
      local_user_options.ca_certificate = &pthis->user_cert_;
      local_user_options.ca_certificate_private_key = &pthis->user_private_key_;

      auto user_cert = certificate::create_new(user_public_key, asymmetric_private_key(), local_user_options);

      transactions::transaction_block_builder playback(sp, t);
      playback.add(
        transactions::create_user_transaction(
          user_object_id,
          user_cert,
          userEmail,
          pthis->user_cert_));

      auto channel_id = dht::dht_object_id::generate_random_id();

      auto read_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
      auto write_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
      auto channel = member_user(pthis->user_cert_, pthis->user_private_key_).create_channel(
        sp,
        playback,
        userName);

      channel->add_writer(
        playback,
        pthis->user_name_,
        member_user(user_cert, asymmetric_private_key()),
        member_user(pthis->user_cert_, pthis->user_private_key_));

      playback.save(
        sp,
        t,
        pthis->user_cert_,
        pthis->user_private_key_);

      auto client = sp.get<dht::network::client>();
      client->save(
        sp,
        t,
        user_object_id,
        user_private_key_der);

      return true;
    }).then([sp]() -> bool {
      return true;
    });
  }
  catch (...) {
    return vds::async_task<bool>::result(false);
  }
}

bool vds::_user_manager::parse_join_request(
  const vds::service_provider &sp,
  const vds::const_data_buffer &data,
  std::string & userName,
  std::string & userEmail) {
  try {
    const_data_buffer user_public_key_der;
    std::string user_object_id;
    const_data_buffer user_private_key_der;

    binary_deserializer s(data);
    s
      >> userName
      >> userEmail
      >> user_public_key_der
      >> user_object_id
      >> user_private_key_der;

    auto pos = s.size();

    const_data_buffer signature;
    s >> signature;

    auto user_public_key = asymmetric_public_key::parse_der(user_public_key_der);

    return asymmetric_sign_verify::verify(
      hash::sha256(),
      user_public_key,
      signature,
      data.data(),
      data.size() - pos);
  }
  catch (...) {
    return false;
  }
}

vds::async_task<vds::user_channel> vds::_user_manager::create_channel(
  const service_provider& sp,
  const std::string& name) {
  auto result = std::make_shared<vds::user_channel>();
  return sp.get<db_model>()->async_transaction(
    sp,
    [pthis = this->shared_from_this(), sp, name, result](database_transaction & t)->bool {

    vds::transactions::transaction_block_builder log(sp, t);
    vds::asymmetric_private_key channel_read_private_key;
    vds::asymmetric_private_key channel_write_private_key;
    *result = member_user(pthis->user_cert_, pthis->user_private_key_).create_channel(
      sp,
      log,
      name);

    log.save(
      sp,
      t,
      pthis->user_cert(),
      pthis->user_private_key());

    pthis->update(sp, t);

    return true;
  })
      .then([result]() {
    return *result;
  });

}
