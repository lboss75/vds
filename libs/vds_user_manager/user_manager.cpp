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
#include "transactions/root_user_transaction.h"
#include "transactions/create_channel_transaction.h"
#include "transactions/channel_add_member_transaction.h"
#include "transactions/channel_add_message_transaction.h"
#include "private/cert_control_p.h"
#include "transaction_context.h"
#include "cert_control.h"
#include "transactions/device_user_add_transaction.h"
#include "vds_exceptions.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/channel_add_reader_transaction.h"
#include "transactions/channel_add_writer_transaction.h"
#include "transactions/user_channel_create_transaction.h"
#include "transaction_log.h"
#include "db_model.h"
#include "certificate_chain_dbo.h"
#include "dht_object_id.h"
#include "channel_local_cache_dbo.h"

vds::user_manager::user_manager(){
}

vds::user_manager::~user_manager() {
}

vds::async_task<> vds::user_manager::update(const service_provider& sp) {
  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this()](database_transaction & t) {
    pthis->impl_->update(sp, t);
    return true;
  });
}

vds::user_manager::login_state_t vds::user_manager::get_login_state() const {
  return this->impl_->get_login_state();
}

void vds::user_manager::load(
  const service_provider & sp,
  database_transaction & t,
  const const_data_buffer & dht_user_id,
  const symmetric_key & user_password_key,
  const const_data_buffer& user_password_hash)
{
	if (nullptr != this->impl_.get()) {
		throw std::runtime_error("Logic error");
	}

  orm::channel_local_cache_dbo t1;
  auto st = t.get_reader(t1.select(t1.last_sync).where(t1.channel_id == base64::from_bytes(dht_user_id)));
  if(!st.execute()) {
    t.execute(t1.insert(
      t1.channel_id = base64::from_bytes(dht_user_id),
      t1.last_sync = std::chrono::system_clock::now()));
  }
  else {
    t.execute(t1.update(t1.last_sync = std::chrono::system_clock::now()).where(
      t1.channel_id == base64::from_bytes(dht_user_id)));
  }


	this->impl_.reset(new _user_manager(
		dht_user_id,
		user_password_key,
    user_password_hash));

	this->impl_->update(sp, t);
}

void vds::user_manager::reset(
    const service_provider &sp,
    database_transaction &t,
    const std::string &root_user_name,
    const std::string &root_password,
    const asymmetric_private_key &root_private_key) {

  transactions::transaction_block playback;
  //Create root user
  auto root_user = this->create_root_user(playback, t, root_user_name, root_password,
                                          root_private_key);

  sp.get<logger>()->info(ThisModule, sp, "Create root user %s. Cert %s", 
	  root_user.id().str().c_str(),
	  cert_control::get_id(root_user.user_certificate()).str().c_str());

  //Create notes channel
  auto channel_read_private_key = vds::asymmetric_private_key::generate(
    vds::asymmetric_crypto::rsa4096());
  auto channel_write_private_key = vds::asymmetric_private_key::generate(
    vds::asymmetric_crypto::rsa4096());
  this->create_channel(
    sp,
    playback,
    t,
    vds::dht::dht_object_id::generate_random_id(),
    user_channel::channel_type_t::notes_channel,
    "My Records",
    root_user.user_certificate(),
    root_private_key,
    channel_read_private_key,
    channel_write_private_key);

  std::list<certificate> certificate_chain;
  certificate_chain.push_back(root_user.user_certificate());

  auto blocks = playback.save_self_signed(
      sp,
      t,
      dht::dht_object_id::from_user_email(root_user_name),
      root_user.user_certificate(),
      root_private_key,
      symmetric_key::from_password(root_password),
      hash::signature(hash::sha256(), root_password.c_str(), root_password.length()));
  //this->load(sp, t, device_user.id());

  //return device_activation(root_user_name, certificate_chain, root_private_key);
}

vds::async_task<> vds::user_manager::init_server(
	const service_provider & parent_sp,
	const std::string &root_user_name,
	const std::string & user_password,
	const std::string & device_name,
	int port)
{
	auto sp = parent_sp.create_scope(__FUNCTION__);
	return sp.get<db_model>()->async_transaction(sp, [this, sp, root_user_name, user_password, device_name, port](database_transaction & t)
	{
		this->load(
				sp,
				t,
				dht::dht_object_id::from_user_email(root_user_name),
				symmetric_key::from_password(user_password),
				hash::signature(hash::sha256(), user_password.c_str(), user_password.length())
		);

//    auto user = this->import_user(*request.certificate_chain().rbegin());
//		transactions::transaction_block log;
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
//				device_user.id(),
//				device_user.user_certificate()));
//
//		auto blocks = log.save(
//			sp, t,
//          user.id(),
//          user.user_certificate(),
//          user.user_certificate(),
//          request.private_key());
		//this->load(sp, t, device_user.id());

		return true;
	});
}

vds::user_channel
vds::user_manager::create_channel(const service_provider &sp, transactions::transaction_block &log,
                                  database_transaction &t, const vds::const_data_buffer &channel_id,
                                  user_channel::channel_type_t channel_type, const std::string &name,
                                  asymmetric_private_key &read_private_key,
                                  asymmetric_private_key &write_private_key) const {
  return this->create_channel(sp, log, t, channel_id, channel_type, name,
                              this->impl_->user_cert(), this->impl_->user_private_key(),
                              read_private_key, write_private_key);
}

vds::user_channel
vds::user_manager::create_channel(const service_provider &sp, transactions::transaction_block &log,
                                  database_transaction &t, const vds::const_data_buffer &channel_id,
                                  user_channel::channel_type_t channel_type, const std::string &name,
                                  const certificate &owner_cert,
                                  const asymmetric_private_key &owner_private_key,
                                  asymmetric_private_key &read_private_key,
                                  asymmetric_private_key &write_private_key) const {

	auto read_id = vds::guid::new_guid();
	auto write_id = vds::guid::new_guid();

	sp.get<logger>()->info(ThisModule, sp, "Create channel %s(%s). Read cert %s. Write cert %s",
    base64::from_bytes(channel_id).c_str(),
		name.c_str(),
		read_id.str().c_str(),
		write_id.str().c_str());

  read_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto read_cert = vds::_cert_control::create_cert(
      read_id,
      "Read Member Certificate " + read_id.str() + " for channel " + base64::from_bytes(channel_id),
      read_private_key,
      owner_cert,
      owner_private_key);

  write_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto write_cert = vds::_cert_control::create_cert(
      write_id,
      "Write Member Certificate " + write_id.str() + " for channel " + base64::from_bytes(channel_id),
      write_private_key,
      owner_cert,
      owner_private_key);

  log.add(
      transactions::channel_create_transaction(
          channel_id,
          std::to_string(channel_type),
          name,
          read_cert,
          read_private_key,
          write_cert,
          write_private_key));

  return user_channel(channel_id, channel_type, name, read_cert, write_cert);
}

vds::member_user vds::user_manager::import_user(const certificate &user_cert) {
  return vds::member_user::import_user(user_cert);
}


vds::certificate vds::user_manager::get_channel_write_cert(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
	return this->impl_->get_channel_write_cert(channel_id);
}


vds::asymmetric_private_key vds::user_manager::get_channel_write_key(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
	return this->impl_->get_channel_write_key(channel_id);
}

vds::asymmetric_private_key vds::user_manager::get_channel_write_key(
		const service_provider & sp,
		const const_data_buffer & channel_id,
		const guid & cert_id) const
{
	return this->impl_->get_channel_write_key(channel_id, cert_id);
}

vds::certificate vds::user_manager::get_channel_read_cert(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
	return this->impl_->get_channel_read_cert(channel_id);
}

vds::asymmetric_private_key vds::user_manager::get_channel_read_key(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
	return this->impl_->get_channel_read_key(channel_id);
}

vds::asymmetric_private_key vds::user_manager::get_channel_read_key(
		const service_provider & sp,
		const const_data_buffer & channel_id,
		const guid & cert_id) const
{
	return this->impl_->get_channel_read_key(channel_id, cert_id);
}

vds::certificate vds::user_manager::get_certificate(
  const service_provider & sp,
  const guid & cert_id) const
{
	return this->impl_->get_certificate(cert_id);
}


vds::user_channel vds::user_manager::get_channel(
  const service_provider & sp,
  const const_data_buffer & channel_id) const
{
  return user_channel(
    channel_id,
    this->impl_->get_channel_type(channel_id),
    this->impl_->get_channel_name(channel_id),
    this->get_channel_read_cert(sp, channel_id),
    this->get_channel_write_cert(sp, channel_id));
}

std::list<vds::user_channel> vds::user_manager::get_channels() const {
  std::list<vds::user_channel> result;

  for (const auto & p : this->impl_->channels()) {
    result.push_back(
      vds::user_channel(
        p.first,
        p.second.type_,
        p.second.name_,
        p.second.read_certificates_.find(p.second.current_read_certificate_)->second,
        p.second.write_certificates_.find(p.second.current_write_certificate_)->second));
  }

  return result;
}

vds::member_user vds::user_manager::create_root_user(
  transactions::transaction_block &playback,
  database_transaction & t,
  const std::string &root_user_name,
  const std::string &root_password,
  const vds::asymmetric_private_key &root_private_key) {
  auto root_user_id = guid::new_guid();
  auto root_user_cert = _cert_control::create_root(
      root_user_id,
      "User " + root_user_name,
      root_private_key);

  playback.add(
      transactions::root_user_transaction(
          root_user_id,
          root_user_cert,
          root_user_name,
          root_private_key.der(std::string()),
          hash::signature(hash::sha256(), root_password.c_str(), root_password.length())));
  playback.add_certificate(root_user_cert);

  return member_user(new _member_user(root_user_id, root_user_cert));
}

vds::async_task<vds::user_channel> vds::user_manager::create_channel(
  const service_provider& sp,
  user_channel::channel_type_t channel_type,
  const std::string& name) const {

  auto result = std::make_shared<vds::user_channel>();
  return sp.get<db_model>()->async_transaction(
    sp,
    [pthis = this->shared_from_this(), sp, channel_type, name, result](database_transaction & t)->bool {
    auto read_private_key = vds::asymmetric_private_key::generate(
      vds::asymmetric_crypto::rsa4096());
    auto write_private_key = vds::asymmetric_private_key::generate(
      vds::asymmetric_crypto::rsa4096());

    vds::transactions::transaction_block log;
    vds::asymmetric_private_key channel_read_private_key;
    vds::asymmetric_private_key channel_write_private_key;
    *result = pthis->create_channel(
      sp,
      log,
      t, vds::dht::dht_object_id::generate_random_id(),
      channel_type,
      name,
      channel_read_private_key, channel_write_private_key);

    log.save(
      sp,
      t,
      pthis->impl_->dht_user_id(),
      pthis->impl_->user_cert(),
      pthis->impl_->user_cert(),
      pthis->impl_->user_private_key());

    pthis->impl_->update(sp, t);

    return true;
  })
  .then([result]() {
    return *result;
  });
}

vds::certificate vds::user_manager::get_channel_write_cert(
		const vds::service_provider &sp,
		const const_data_buffer &channel_id,
		const vds::guid &cert_id) const {
	return this->impl_->get_channel_write_cert(channel_id, cert_id);
}

bool vds::user_manager::validate_and_save(
		const service_provider & sp,
		const std::list<vds::certificate> &cert_chain) {

  certificate_store store;
  for (const auto & p : cert_chain) {
    auto cert = this->get_certificate(sp, cert_control::get_id(p));
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
          t1.id = cert_control::get_id(cert),
      t1.cert = cert.der(),
      t1.parent = cert_control::get_parent_id(cert)));

  orm::certificate_unknown_dbo t2;
  t.execute(t2.delete_if(t2.id == cert_control::get_id(cert)));
}

void vds::user_manager::create_root_user(
		const service_provider &sp,
		database_transaction &t,
		const std::string &user_email,
		const symmetric_key &password_key,
		const const_data_buffer &password_hash) {

	auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
	auto user_id = guid::new_guid();
	auto user_cert = _cert_control::create_root(
			user_id,
			"User " + user_email,
			private_key);

	transactions::transaction_block playback;
	playback.add(
			transactions::root_user_transaction(
					user_id,
					user_cert,
					user_email,
					symmetric_encrypt::encrypt(password_key, private_key.der(std::string())),
					password_hash));

	playback.save(
			sp,
			t,
			dht::dht_object_id::from_user_email(user_email),
			user_cert,
			user_cert,private_key);


}

void vds::user_manager::save_certificate(const vds::service_provider &sp, const vds::certificate &cert) {
  this->impl_->add_certificate(cert);

  sp.get<db_model>()->async_transaction(sp, [cert](database_transaction & t)->bool{

    orm::certificate_chain_dbo t1;
    auto st = t.get_reader(t1.select(t1.id).where(t1.id == cert_control::get_id(cert)));
    if(!st.execute()){
      t.execute(t1.insert(
          t1.id = cert_control::get_id(cert),
          t1.cert = cert.der(),
          t1.parent = cert_control::get_parent_id(cert)));
    }

    return true;
  }).execute([sp](const std::shared_ptr<std::exception> & ex){
    if(ex) {
      sp.get<logger>()->warning(ThisModule, sp, "%s at saving certificate", ex->what());
    }
  });
}

const vds::const_data_buffer &vds::user_manager::dht_user_id() const {
	return this->impl_->dht_user_id();
}


/////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager(
		const const_data_buffer & dht_user_id,
		const symmetric_key & user_password_key,
		const const_data_buffer & user_password_hash)
		: dht_user_id_(dht_user_id),
			user_password_key_(user_password_key),
			user_password_hash_(user_password_hash),
			login_state_(user_manager::login_state_t::waiting_channel){
}

void vds::_user_manager::update(
		const service_provider & parent_scope,
		database_transaction &t) {
	auto sp = parent_scope.create_scope(__FUNCTION__);
	const auto log = sp.get<logger>();
	log->trace(ThisModule, sp, "security_walker::load");

	orm::transaction_log_record_dbo t1;
	auto st = t.get_reader(
			t1.select(t1.id, t1.data)
					.where(t1.channel_id == base64::from_bytes(this->dht_user_id_))
					.order_by(t1.order_no));

	std::set<const_data_buffer> new_channels;
	while (st.execute()) {
		auto id = t1.id.get(st);
		if (this->processed_.end() != this->processed_.find(id)) {
			continue;;
		}

		const_data_buffer channel_id;
		uint64_t order_no;
		guid read_cert_id;
		guid write_cert_id;
		std::set<const_data_buffer> ancestors;
		const_data_buffer crypted_data;
		const_data_buffer crypted_key;
		const_data_buffer signature;
		std::list<certificate> certificates;

		auto block_type = transactions::transaction_block::parse_block(
				t1.data.get(st),
				channel_id,
				order_no,
				read_cert_id,
				write_cert_id,
				ancestors,
				crypted_data,
				crypted_key,
				certificates,
				signature);

		if (channel_id != this->dht_user_id_) {
			throw std::runtime_error("Ivalid record");
		}

		certificate write_cert;
		auto pcert = this->certificate_chain_.find(write_cert_id);
		if(this->certificate_chain_.end() == pcert) {
			for(auto & cert : certificates) {
				if(write_cert_id == cert_control::get_id(cert)) {
					write_cert = cert;
					break;
				}
			}
		} else {
			write_cert = pcert->second;
		}

		if (!write_cert || !transactions::transaction_block::validate_block(
				write_cert,
				block_type,
				channel_id,
				order_no,
				read_cert_id,
				write_cert_id,
				ancestors,
				crypted_data,
				crypted_key,
				certificates,
				signature)) {
			log->error(ThisModule, sp, "Write signature error");
			throw std::runtime_error("Write signature error");
		}

		const_data_buffer data;
		switch (block_type) {
			case transactions::transaction_block::block_type_t::normal: {
				const auto key_data = this->user_private_key_.decrypt(crypted_key);
				const auto key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), key_data);
				data = symmetric_decrypt::decrypt(key, crypted_data);
				break;
			}
			case transactions::transaction_block::block_type_t::self_signed: {
				if(this->user_password_hash_ != crypted_key) {
					this->login_state_ = user_manager::login_state_t::login_failed;
				}
				else {
					data = symmetric_decrypt::decrypt(this->user_password_key_, crypted_data);
				}
				break;
			}
			default:
				throw std::runtime_error("Invalid data");
		}

		binary_deserializer s(data);

		while (0 < s.size()) {
			uint8_t message_id;
			s >> message_id;

			switch ((transactions::transaction_id) message_id) {
				case transactions::transaction_id::root_user_transaction: {
					if(this->login_state_ != user_manager::login_state_t::waiting_channel) {
						throw std::runtime_error("Data error");
					}

					transactions::root_user_transaction message(s);
					this->user_id_ = message.id();
					this->user_cert_ = message.user_cert();
					this->user_name_ = message.user_name();
					this->user_private_key_ = asymmetric_private_key::parse_der(message.user_private_key(), std::string());

					this->login_state_ = user_manager::login_state_t::login_sucessful;
					break;
				}
				case transactions::transaction_id::channel_add_reader_transaction: {
					transactions::channel_add_reader_transaction message(s);
					auto &cp = this->channels_[message.channel_id()];

					cp.name_ = message.name();

					auto write_id = cert_control::get_id(message.write_cert());
					cp.write_certificates_[write_id] = message.write_cert();

					auto id = cert_control::get_id(message.read_cert());
					cp.read_certificates_[id] = message.read_cert();
					cp.read_private_keys_[id] = message.read_private_key();
					cp.current_read_certificate_ = id;

					log->debug(ThisModule, sp, "Got channel %s reader certificate %s",
										 base64::from_bytes(message.channel_id()).c_str(),
										 id.str().c_str());

					break;
				}

				case transactions::transaction_id::channel_add_writer_transaction: {
					transactions::channel_add_writer_transaction message(s);
					auto &cp = this->channels_[message.channel_id()];
					auto id = cert_control::get_id(message.write_cert());
					cp.name_ = message.name();
					cp.write_certificates_[id] = message.write_cert();
					cp.write_private_keys_[id] = message.write_private_key();
					cp.current_write_certificate_ = id;

					auto read_id = cert_control::get_id(message.read_cert());
					cp.read_certificates_[read_id] = message.read_cert();
					cp.current_read_certificate_ = read_id;

					log->debug(ThisModule, sp, "Got channel %s write certificate %s, read certificate %s",
										 base64::from_bytes(message.channel_id()).c_str(),
										 id.str().c_str(),
										 read_id.str().c_str());

					break;
				}

				case transactions::transaction_id::channel_create_transaction: {
					transactions::channel_create_transaction message(s);
					if(new_channels.end() == new_channels.find(message.channel_id())) {
						new_channels.emplace(message.channel_id());
					}
					auto &cp = this->channels_[message.channel_id()];
					cp.name_ = message.name();
					cp.current_read_certificate_ = cert_control::get_id(message.read_cert());
					cp.current_write_certificate_ = cert_control::get_id(message.write_cert());
					cp.read_certificates_[cp.current_read_certificate_] = message.read_cert();
					cp.read_private_keys_[cp.current_read_certificate_] = message.read_private_key();
					cp.write_certificates_[cp.current_write_certificate_] = message.write_cert();
					cp.write_private_keys_[cp.current_write_certificate_] = message.write_private_key();

					log->debug(ThisModule, sp, "New channel %s(%s), read certificate %s, write certificate %s",
										 base64::from_bytes(message.channel_id()).c_str(),
										 message.name().c_str(),
										 cp.current_read_certificate_.str().c_str(),
										 cp.current_write_certificate_.str().c_str());

					break;
				}

				case transactions::transaction_id::device_user_add_transaction: {
					transactions::device_user_add_transaction message(s);
					break;
				}

				default:
					throw std::runtime_error("logic error");
			}
		}
		this->processed_.emplace(id);
	}

	for(auto & channel_id : new_channels){
		orm::channel_local_cache_dbo t1;
		auto st = t.get_reader(t1.select(t1.last_sync).where(t1.channel_id == base64::from_bytes(channel_id)));
		if(st.execute()){
			t.execute(
					t1.update(
							t1.last_sync = std::chrono::system_clock::now())
							.where(t1.channel_id == base64::from_bytes(channel_id)));
		}
		else {
			t.execute(
					t1.insert(
							t1.channel_id = base64::from_bytes(channel_id),
							t1.last_sync = std::chrono::system_clock::now()));
		}
	}
}

bool
vds::_user_manager::get_channel_write_certificate(
		const guid &channel_id,
		std::string & name,
		certificate & read_certificate,
		asymmetric_private_key &read_key,
		certificate & write_certificate,
		asymmetric_private_key &write_key) const {
	auto p = this->channels_.find(channel_id);
	if(this->channels_.end() == p){
		return false;
	}

	name = p->second.name_;

	if (p->second.current_read_certificate_) {
		auto p1 = p->second.read_certificates_.find(p->second.current_read_certificate_);
		if (p->second.read_certificates_.end() != p1) {
			read_certificate = p1->second;
		}
		auto p2 = p->second.read_private_keys_.find(p->second.current_read_certificate_);
		if (p->second.read_private_keys_.end() != p2) {
			read_key = p2->second;
		}
	}

	if (p->second.current_write_certificate_) {
		auto p1 = p->second.write_certificates_.find(p->second.current_write_certificate_);
		if (p->second.write_certificates_.end() != p1) {
			write_certificate = p1->second;
		}
		auto p2 = p->second.write_private_keys_.find(p->second.current_write_certificate_);
		if (p->second.write_private_keys_.end() != p2) {
			write_key = p2->second;
		}
	}

	return true;
}

void vds::_user_manager::add_certificate(const vds::certificate &cert) {
	this->certificate_chain_[cert_control::get_id(cert)] = cert;
}

