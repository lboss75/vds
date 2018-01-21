/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_manager.h"
#include "member_user.h"
#include "user_manager_storage.h"
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
#include "channel_message.h"
#include "transactions/device_user_add_transaction.h"
#include "run_configuration_dbo.h"
#include "vds_exceptions.h"
#include "transactions/channel_create_transaction.h"
#include "transactions/channel_add_reader_transaction.h"
#include "transactions/channel_add_writer_transaction.h"
#include "transactions/user_channel_create_transaction.h"
#include "transaction_log.h"
#include "db_model.h"
#include "certificate_chain_dbo.h"

vds::user_manager::user_manager()
{
}

void vds::user_manager::load(
	const service_provider & sp,
	database_transaction & t)
{
	if (nullptr != this->impl_.get()) {
		throw std::runtime_error("Logic error");
	}

	dbo::run_configuration t1;
	auto st = t.get_reader(t1.select(t1.cert, t1.cert_private_key, t1.common_channel_id, t1.common_channel_read_cert, t1.common_channel_pkey));
	if (!st.execute()) {
		throw std::runtime_error("Unable to get current configuration");
	}

	auto common_channel_id = t1.common_channel_id.get(st);
	auto device_cert = certificate::parse_der(t1.cert.get(st));
	auto device_id = cert_control::get_user_id(device_cert);
	auto device_private_key = asymmetric_private_key::parse_der(t1.cert_private_key.get(st), std::string());
	auto common_channel_read_cert = certificate::parse_der(t1.common_channel_read_cert.get(st));
	auto common_read_private_key = asymmetric_private_key::parse_der(t1.common_channel_pkey.get(st), std::string());

	if (st.execute()) {
		throw std::runtime_error("Multiple config");
	}

	this->impl_.reset(new _user_manager(common_channel_id, device_id, device_cert, device_private_key));
	this->impl_->add_read_certificate(common_channel_id, common_channel_read_cert, common_read_private_key);
	this->impl_->load(sp, t);
}

vds::user_invitation vds::user_manager::reset(
    const service_provider &sp,
    database_transaction &t,
    const std::string &root_user_name,
    const std::string &root_password,
    const asymmetric_private_key &root_private_key,
    const std::string &device_name,
    int port) {

  guid common_channel_id = guid::new_guid();
  transactions::transaction_block playback;
  //Create root user
  auto root_user = this->create_root_user(playback, t, common_channel_id, root_user_name, root_password,
                                          root_private_key);

  sp.get<logger>()->info(ThisModule, sp, "Create root user %s. Cert %s", 
	  root_user.id().str().c_str(),
	  cert_control::get_id(root_user.user_certificate()).str().c_str());

  //Create common channel
  asymmetric_private_key common_read_private_key;
  asymmetric_private_key common_write_private_key;
  auto common_channel = this->create_channel(
	  sp,
	  playback,
	  t,
	  common_channel_id,
	  "Common channel",
	  root_user.id(),
      root_user.user_certificate(),
      root_private_key,
	  common_read_private_key,
	  common_write_private_key);

  //Lock to device
  std::list<certificate> certificate_chain;
  certificate_chain.push_back(root_user.user_certificate());
  auto device_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  auto device_user = this->lock_to_device(
      sp, t, playback,
	  certificate_chain,
	  root_user, root_user_name, root_password, root_private_key,
      device_name, device_key, common_channel_id, port,
	  common_channel.read_cert(), common_read_private_key);

  playback.add(
      transactions::device_user_add_transaction(
          device_user.id(),
          device_user.user_certificate()));
  //
  common_channel.add_reader(
	  playback,
	  device_user,
	  root_user,
	  root_private_key,
	  common_read_private_key);

  common_channel.add_writer(
	  playback,
	  device_user,
	  root_user,
	  root_private_key,
	  common_write_private_key);

  auto block_id = playback.save(
      sp,
      t,
      common_channel.read_cert(),
      root_user.user_certificate(),
      root_private_key,
	  false);
  this->load(sp, t);
  transaction_log::apply_block(sp, t, block_id);

  return user_invitation(
	  root_user.id(),
	  root_user_name,
	  root_user.user_certificate(),
	  root_private_key,
	  common_channel_id,
	  common_channel.read_cert(),
	  common_read_private_key,
	  common_channel.write_cert(),
	  common_write_private_key,
	  std::list<certificate>());
}

vds::async_task<> vds::user_manager::init_server(
	const service_provider & parent_sp,
	const user_invitation & request,
	const std::string & user_password,
	const std::string & device_name,
	int port)
{
	auto sp = parent_sp.create_scope(__FUNCSIG__);
	return sp.get<db_model>()->async_transaction(sp, [this, sp, request, user_password, device_name, port](database_transaction & t)
	{
		sp.get<logger>()->info("UDPAPI", sp, "Read certificate %s for channel %s",
			cert_control::get_id(request.common_channel_read_cert()).str().c_str(),
			request.common_channel_id().str().c_str());

		//save certificates
		dbo::certificate_chain_dbo t1;
		for (auto &cert : request.certificate_chain()) {
			sp.get<logger>()->info(ThisModule, sp, "Stored certificate %s", cert_control::get_id(cert).str().c_str());
			t.execute(
				t1.insert(
					t1.id = cert_control::get_id(cert),
					t1.cert = cert.der(),
					t1.parent = cert_control::get_parent_id(cert)
				));
		}
		auto user = request.get_user();
		sp.get<logger>()->info(ThisModule, sp, "Stored certificate %s", cert_control::get_id(user.user_certificate()).str().c_str());
		t.execute(
			t1.insert(
				t1.id = cert_control::get_id(user.user_certificate()),
				t1.cert = user.user_certificate().der(),
				t1.parent = cert_control::get_parent_id(user.user_certificate())
			));

		transactions::transaction_block log;

		auto private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
		auto device_user = this->lock_to_device(
			sp,
			t,
			log,
			request.certificate_chain(),
			user,
			request.get_user_name(),
			user_password,
			request.get_user_private_key(),
			device_name, 
			private_key,
			request.common_channel_id(),
			port,
			request.common_channel_read_cert(),
			request.common_channel_read_private_key());

		user_channel channel(
			request.common_channel_id(),
			"Common channel",
			request.common_channel_read_cert(),
			request.common_channel_write_cert());

		sp.get<logger>()->trace(
			"UDPAPI",
			sp,
			"Allow read/write common channel (%s->%s) by local device user(%s->%s)",
			request.common_channel_id().str().c_str(),
			cert_control::get_id(request.common_channel_read_cert()).str().c_str(),
			device_user.id().str().c_str(),
			cert_control::get_id(device_user.user_certificate()).str().c_str());

		channel.add_reader(
			log,
			device_user,
			user, 
			request.get_user_private_key(),
			request.common_channel_read_private_key());

		channel.add_writer(
			log,
			device_user,
			user,
			request.get_user_private_key(),
			request.common_channel_write_private_key());

		log.add(
			transactions::device_user_add_transaction(
				device_user.id(),
				device_user.user_certificate()));

		auto block_id = log.save(
			sp, t,
			request.common_channel_read_cert(),
			user.user_certificate(),
			request.get_user_private_key(),
			false);
		this->load(sp, t);
		transaction_log::apply_block(sp, t, block_id);

		return true;
	});
}

vds::user_channel
vds::user_manager::create_channel(
	const service_provider & sp,
	transactions::transaction_block &log,
	database_transaction &t,
    const vds::guid &channel_id,
	const std::string &name,
    const vds::guid &owner_id,
	const certificate &owner_cert,
    const asymmetric_private_key &owner_private_key,
	asymmetric_private_key &read_private_key,
	asymmetric_private_key &write_private_key) const {

	auto read_id = vds::guid::new_guid();
	auto write_id = vds::guid::new_guid();

	sp.get<logger>()->info(ThisModule, sp, "Create channel %s(%s). Read cert %s. Write cert %s",
		channel_id.str().c_str(),
		name.c_str(),
		read_id.str().c_str(),
		write_id.str().c_str());

  read_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto read_cert = vds::_cert_control::create_cert(
      read_id,
      "Read Member Certificate " + read_id.str() + " for channel " + channel_id.str(),
      read_private_key,
      owner_cert,
      owner_private_key);

  write_private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa4096());
  auto write_cert = vds::_cert_control::create_cert(
      write_id,
      "Write Member Certificate " + write_id.str() + " for channel " + channel_id.str(),
      write_private_key,
      owner_cert,
      owner_private_key);

  log.add(
      transactions::channel_create_transaction(
          owner_id,
          owner_cert,
          cert_control::get_id(owner_cert),
          owner_private_key,
          channel_id,
          name,
          read_cert,
          read_private_key,
          write_cert,
          write_private_key));

  return user_channel(channel_id, name, read_cert, write_cert);
}

vds::member_user
vds::user_manager::lock_to_device(
	const vds::service_provider &sp,
	vds::database_transaction &t,
	transactions::transaction_block & playback,
	const std::list<certificate> & certificate_chain,
                                  const member_user &user,
								  const std::string &user_name,
                                  const std::string &user_password,
                                  const asymmetric_private_key &user_private_key,
                                  const std::string &device_name,
                                  const asymmetric_private_key &device_private_key,
                                  const guid &common_channel_id, int port,
	const certificate & common_channel_read_cert,
	const asymmetric_private_key & common_channel_pkey) {

  auto device_user = user.create_device_user(
      user_private_key,
      device_private_key,
      device_name);

  auto config_id = guid::new_guid();
  dbo::run_configuration t3;
  t.execute(
      t3.insert(
          t3.id = config_id,
          t3.cert = device_user.user_certificate().der(),
          t3.cert_private_key = device_private_key.der(std::string()),
          t3.port = port,
          t3.common_channel_id = common_channel_id,
		  t3.common_channel_read_cert = common_channel_read_cert.der(),
		  t3.common_channel_pkey = common_channel_pkey.der(std::string())));
  dbo::certificate_chain_dbo t4;
  for(auto & cert : certificate_chain)
  {
	  t.execute(
		  t4.insert(
			  t4.id = cert_control::get_id(cert),
			  t4.cert = cert.der(),
			  t4.parent = cert_control::get_parent_id(cert)));
  }
  t.execute(
	  t4.insert(
		  t4.id = cert_control::get_id(device_user.user_certificate()),
		  t4.cert = device_user.user_certificate().der(),
		  t4.parent = cert_control::get_parent_id(device_user.user_certificate())));

  return device_user;
}

vds::member_user vds::user_manager::import_user(const certificate &user_cert) {
  return vds::member_user::import_user(user_cert);
}

vds::member_user vds::user_manager::get_current_device(
    const vds::service_provider &sp,
    asymmetric_private_key &device_private_key) const {

	return this->impl_->get_current_device(sp, device_private_key);
}

vds::certificate vds::user_manager::get_channel_write_cert(const guid & channel_id) const
{
	return this->impl_->get_channel_write_cert(channel_id);
}

vds::asymmetric_private_key vds::user_manager::get_channel_write_key(const guid & channel_id) const
{
	return this->impl_->get_channel_write_key(channel_id);
}

vds::certificate vds::user_manager::get_channel_read_cert(const guid & channel_id) const
{
	return this->impl_->get_channel_read_cert(channel_id);
}

vds::asymmetric_private_key vds::user_manager::get_channel_read_key(const guid & channel_id) const
{
	return this->impl_->get_channel_read_key(channel_id);
}

vds::certificate vds::user_manager::get_certificate(const guid & cert_id) const
{
	return this->impl_->get_certificate(cert_id);
}

vds::asymmetric_private_key vds::user_manager::get_common_channel_read_key(const guid & cert_id) const
{
	return this->impl_->get_common_channel_read_key(cert_id);
}


vds::user_channel vds::user_manager::get_channel(const guid & channel_id) const
{
	return this->impl_->get_channel(channel_id);
}

vds::user_channel vds::user_manager::get_common_channel() const {
	auto id = this->impl_->get_common_channel_id();
  return this->get_channel(id);
}

vds::member_user vds::user_manager::create_root_user(transactions::transaction_block &playback, database_transaction &t,
                                                     const guid &common_channel_id, const std::string &root_user_name,
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
          root_private_key.der(root_password),
          hash::signature(hash::sha256(), root_password.c_str(), root_password.length())));

  return member_user(new _member_user(root_user_id, root_user_cert));
}

void vds::user_manager::apply_channel_message(
	const service_provider & sp,
	const guid & channel_id,
	int message_id,
	const guid & read_cert_id,
	const guid & write_cert_id,
	const const_data_buffer & message,
	const const_data_buffer & signature)
{
	this->impl_->apply(
		sp,
		channel_id,
		message_id,
		read_cert_id,
		write_cert_id,
		message,
		signature);
}

vds::guid vds::user_manager::get_common_channel_id() const
{
	return this->impl_->get_common_channel_id();
}

////////////////////////////////////////////////////////////////////////
vds::_user_manager::_user_manager(
	const guid & common_channel_id,
	const guid & user_id,
	const certificate & user_cert,
	const asymmetric_private_key & user_private_key)
	: security_walker(common_channel_id, user_id, user_cert, user_private_key)
{
}

vds::member_user vds::_user_manager::get_current_device(const service_provider & sp, asymmetric_private_key & device_private_key) const
{
	device_private_key = this->user_private_key();
	return member_user(
		new _member_user(
			this->user_id(),
			this->user_cert()));
}

vds::user_channel vds::_user_manager::get_channel(const guid & channel_id) const
{
	return user_channel(
		channel_id,
		this->get_channel_name(channel_id),
		this->get_channel_read_cert(channel_id),
		this->get_channel_write_cert(channel_id));
}
