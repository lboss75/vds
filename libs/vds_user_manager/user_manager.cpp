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
#include "user_manager_transactions.h"
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
#include "control_message_transaction.h"
#include "create_user_transaction.h"

vds::user_manager::user_manager(const service_provider * sp)
: sp_(sp) {
}

vds::user_manager::~user_manager() {
}

vds::user_manager::login_state_t vds::user_manager::get_login_state() const {
  return this->impl_->get_login_state();
}

vds::async_task<vds::expected<void>> vds::user_manager::update() {
  return this->sp_->get<db_model>()->async_read_transaction([pthis = this->shared_from_this()](database_read_transaction & t) -> expected<void> {
    return pthis->update(t);
  });
}

vds::expected<void> vds::user_manager::update(database_read_transaction & t) const {
  return this->impl_->update(t);
}

vds::expected<void> vds::user_manager::load(
  database_read_transaction & t,
  const std::string & user_login,
  const std::string & user_password)
{
	if (nullptr != this->impl_.get()) {
		return vds::make_unexpected<std::runtime_error>("Logic error");
	}

  this->impl_.reset(new _user_manager());
  CHECK_EXPECTED(this->impl_->create(
    this->sp_,
    user_login,
    user_password));

	return this->impl_->update(t);
}

vds::async_task<vds::expected<vds::user_channel>> vds::user_manager::create_channel(
  const std::string & channel_type,
  const std::string& name) const {
  return this->impl_->create_channel(
    channel_type,
    name);
}


vds::expected<void> vds::user_manager::reset(
    const std::string &root_user_name,
    const std::string &root_password,
    const cert_control::private_info_t & private_info) {
  return this->sp_->get<db_model>()->async_transaction([this, root_user_name, root_password, private_info](
    database_transaction & t) -> expected<void> {

    auto playback = transactions::transaction_block_builder::create_root_block(this->sp_);

    //Create root user
    GET_EXPECTED(root_user, _member_user::create_root_user(
      playback,
      root_user_name,
      root_password,
      private_info.root_private_key_));

    //common news
    this->sp_->get<logger>()->info(ThisModule, "Create channel %s(Common News)",
      base64::from_bytes(cert_control::get_common_news_channel_id()).c_str());

    GET_EXPECTED(pc, root_user->personal_channel());
    CHECK_EXPECTED(pc.add_log(
      playback,
      message_create<transactions::channel_create_transaction>(
        cert_control::get_common_news_channel_id(),
        user_channel::channel_type_t::news_channel,
        "Common news",
        cert_control::get_common_news_read_certificate(),
        cert_control::get_common_news_read_private_key(),
        cert_control::get_common_news_write_certificate(),
        private_info.common_news_write_private_key_)));

    //Auto update
    this->sp_->get<logger>()->info(ThisModule, "Create channel %s(Common News)",
      base64::from_bytes(cert_control::get_autoupdate_channel_id()).c_str());

    CHECK_EXPECTED(pc.add_log(
      playback,
      message_create<transactions::channel_create_transaction>(
        cert_control::get_autoupdate_channel_id(),
        user_channel::channel_type_t::news_channel,
        "Auto update",
        cert_control::get_autoupdate_read_certificate(),
        cert_control::get_autoupdate_read_private_key(),
        cert_control::get_autoupdate_write_certificate(),
        private_info.autoupdate_write_private_key_)));

    CHECK_EXPECTED(playback.save(
      this->sp_,
      t,
      root_user.user_certificate(),
      private_info.root_private_key_));

    return expected<void>();
  }).get();
}

std::shared_ptr<vds::user_channel> vds::user_manager::get_channel(
  const const_data_buffer & channel_id) const
{
  return this->impl_->get_channel(channel_id);
}

std::map<vds::const_data_buffer, std::shared_ptr<vds::user_channel>> vds::user_manager::get_channels() const {
  std::list<vds::user_channel> result;

  return this->impl_->channels();
}

//vds::expected<bool> vds::user_manager::validate_and_save(
//		
//		const std::list<std::shared_ptr<vds::certificate>> &cert_chain) {
//
//  certificate_store store;
//  for (const auto & p : cert_chain) {
//    auto cert = this->impl_->get_certificate(p->subject());
//    if (!cert) {
//      cert = p;
//
//      GET_EXPECTED(result, store.verify(*cert));
//      if (0 != result.error_code) {
//        this->sp_->get<logger>()->warning(ThisModule, "Invalid certificate %s %s",
//          result.error.c_str(),
//          result.issuer.c_str());
//        return false;
//      }
//    }
//
//    CHECK_EXPECTED(store.add(*cert));
//    CHECK_EXPECTED(this->save_certificate(cert));
//  }
//
//  return true;
//}
//
vds::expected<void> vds::user_manager::save_certificate(
    
    vds::database_transaction &t,
    const vds::certificate &cert) {

  orm::certificate_chain_dbo t1;
  GET_EXPECTED(st, t.get_reader(t1.select(t1.id).where(t1.id == cert.subject())));
  GET_EXPECTED(st_result, st.execute());
  if (!st_result) {
    GET_EXPECTED(der, cert.der());
    CHECK_EXPECTED(t.execute(t1.insert(
      t1.id = cert.subject(),
      t1.cert = der,
      t1.parent = cert.issuer())));
  }

  orm::certificate_unknown_dbo t2;
  return t.execute(t2.delete_if(t2.id == cert.subject()));
}

vds::async_task<vds::expected<void>> vds::user_manager::save_certificate( const std::shared_ptr<vds::certificate> &cert) {
  this->impl_->add_certificate(cert);

  return this->sp_->get<db_model>()->async_transaction([cert](database_transaction & t) -> expected<void> {
    return save_certificate(t, *cert);
  });
}

vds::member_user vds::user_manager::get_current_user() const {
  return this->impl_->get_current_user();
}

const std::shared_ptr<vds::asymmetric_private_key> & vds::user_manager::get_current_user_private_key() const {
  return this->impl_->get_current_user_private_key();
}

vds::expected<vds::const_data_buffer> vds::user_manager::create_register_request(
  const service_provider * /*sp*/,
  const std::string& userName,
  const std::string& user_email,
  const std::string& user_password) {

  GET_EXPECTED(user_private_key, vds::asymmetric_private_key::generate(
    vds::asymmetric_crypto::rsa4096()));

  GET_EXPECTED(user_public_key, asymmetric_public_key::create(user_private_key));

  binary_serializer s;
  CHECK_EXPECTED(s << userName);
  CHECK_EXPECTED(s << user_email);
  CHECK_EXPECTED(s << user_public_key.der());
  CHECK_EXPECTED(s << dht::dht_object_id::user_credentials_to_key(user_email, user_password));
  CHECK_EXPECTED(s << user_private_key.der(user_password));

  CHECK_EXPECTED(s << asymmetric_sign::signature(hash::sha256(), user_private_key, s.get_buffer(), s.size()));

  return s.move_data();
}

vds::expected<bool> vds::user_manager::parse_join_request(
  const vds::const_data_buffer &data,
  std::string &userName,
  std::string &userEmail) {
  return _user_manager::parse_join_request(data, userName, userEmail);
}

vds::async_task<vds::expected<bool>> vds::user_manager::approve_join_request(
  const const_data_buffer& data) {
  return this->impl_->approve_join_request(data);
}

const std::list<std::shared_ptr<vds::user_wallet>>& vds::user_manager::wallets() const
{
  return this->impl_->wallets();
}

/////////////////////////////////////////////////////////////////////
vds::expected<void> vds::_user_manager::create(
  const service_provider * sp,
		const std::string & user_login,
    const std::string & user_password) {
  this->sp_ = sp;
  this->login_state_ = user_manager::login_state_t::waiting;
  GET_EXPECTED_VALUE(this->user_credentials_key_, dht::dht_object_id::user_credentials_to_key(user_login, user_password));
  this->user_password_ = user_password;
  return expected<void>();
}

vds::expected<bool> vds::_user_manager::process_root_user_transaction(const transactions::root_user_transaction & message) {
  this->root_user_cert_ = message.user_cert;
  this->root_user_name_ = message.user_name;

  if (this->user_credentials_key_ == message.user_credentials_key) {
    this->user_cert_ = message.user_cert;
    this->user_name_ = message.user_name;
    this->login_state_ = user_manager::login_state_t::login_successful;

    GET_EXPECTED(user_private_key, asymmetric_private_key::parse_der(message.user_private_key, this->user_password_));
    this->user_private_key_ = std::make_shared<asymmetric_private_key>(std::move(user_private_key));

    GET_EXPECTED(cp, _user_channel::import_personal_channel(
      this->user_cert_,
      this->user_private_key_));

    this->channels_[cp->id()] = cp;

    GET_EXPECTED_VALUE(cp, _user_channel::import_personal_channel(
      this->user_cert_,
      this->user_private_key_));

    this->channels_[cp->id()] = cp;
  }

  return true;
}

vds::expected<bool> vds::_user_manager::process_create_user_transaction(
  const transactions::create_user_transaction & message) {
  if (this->user_credentials_key_ == message.user_credentials_key) {
    this->user_cert_ = message.user_cert;
    this->user_name_ = message.user_name;

    GET_EXPECTED(user_private_key, asymmetric_private_key::parse_der(message.user_private_key, this->user_password_));
    this->user_private_key_ = std::make_shared<asymmetric_private_key>(std::move(user_private_key));

    this->login_state_ = user_manager::login_state_t::login_successful;

    GET_EXPECTED(cp, _user_channel::import_personal_channel(
      this->user_cert_,
      this->user_private_key_));
    this->channels_[cp->id()] = cp;
  }
  return true;
}

vds::expected<bool> vds::_user_manager::process_channel_message(
  const transactions::channel_message & message,
  std::set<const_data_buffer> & new_channels,
  std::chrono::system_clock::time_point tp) {
  const auto log = this->sp_->get<logger>();
  auto channel = this->get_channel(message.channel_id());
  if (channel) {
    auto channel_read_key = channel->read_cert_private_key(message.channel_read_cert_subject());
    if (channel_read_key) {
      CHECK_EXPECTED(message.walk_messages(this->sp_, *channel_read_key, transactions::message_environment_t{ tp, "???" },
        [this, channel_id = message.channel_id(), log](
          const transactions::channel_add_reader_transaction & message,
          const transactions::message_environment_t & /*message_environment*/)->expected<bool> {
        auto cp = std::make_shared<user_channel>(
          message.id,
          message.channel_type,
          message.name,
          message.read_cert,
          message.read_private_key,
          message.write_cert,
          std::shared_ptr<asymmetric_private_key>());

        this->channels_[cp->id()] = cp;
        log->debug(ThisModule, "Got channel %s reader certificate",
          base64::from_bytes(cp->id()).c_str());

        return true;
      },
        [this, channel_id = message.channel_id(), log](
          const transactions::channel_add_writer_transaction & message,
          const transactions::message_environment_t & /*message_environment*/)->expected<bool> {
        auto cp = std::make_shared<user_channel>(
          message.id,
          message.channel_type,
          message.name,
          message.read_cert,
          message.read_private_key,
          message.write_cert,
          message.write_private_key);

        this->channels_[cp->id()] = cp;

        log->debug(ThisModule, "Got channel %s write certificate",
          base64::from_bytes(cp->id()).c_str());

        return true;
      },
        [this, channel_id = message.channel_id(), log, &new_channels](
          const transactions::channel_create_transaction & message,
          const transactions::message_environment_t & /*message_environment*/)->expected<bool>{
        if (new_channels.end() == new_channels.find(channel_id)) {
          new_channels.emplace(message.channel_id);
        }
        auto cp = std::make_shared<user_channel>(
          message.channel_id,
          message.channel_type,
          message.name,
          message.read_cert,
          message.read_private_key,
          message.write_cert,
          message.write_private_key);

        this->channels_[cp->id()] = cp;

        return true;
      },
        [this, channel_id = message.channel_id(), log](
          const transactions::control_message_transaction & message,
          const transactions::message_environment_t & /*message_environment*/)->expected<bool> {
        auto msg = std::dynamic_pointer_cast<json_object>(message.message);
        std::string type;
        if (msg) {
          GET_EXPECTED(have_type, msg->get_property("$type", type));
          if (have_type) {
            if (transactions::control_message_transaction::create_wallet_type == type) {
              std::string name;
              CHECK_EXPECTED(msg->get_property("name", name));

              GET_EXPECTED(cert, certificate::parse_der(message.attachments.at("cert")));
              GET_EXPECTED(private_key, asymmetric_private_key::parse_der(message.attachments.at("key"), std::string()));
              auto wallet = std::make_shared<user_wallet>(
                name,
                std::move(cert),
                std::move(private_key));

              this->wallets_.push_back(wallet);

              log->debug(ThisModule, "Got wallet %s write certificate",
                name.c_str());
            }
          }
        }

        return true;
      }));
    }
  }

  return true;
}

vds::expected<void> vds::_user_manager::update(
  database_read_transaction &t) {
  const auto log = this->sp_->get<logger>();

  std::list<const_data_buffer> new_records;
  orm::transaction_log_record_dbo t1;
  GET_EXPECTED(st, t.get_reader(
    t1.select(t1.id)
    .order_by(t1.order_no)));
  WHILE_EXPECTED(st.execute())
    auto id = t1.id.get(st);
  if (this->processed_.end() != this->processed_.find(id)) {
    continue;
  }

  new_records.push_back(id);
  WHILE_EXPECTED_END()

    if (new_records.empty() && this->login_state_ == user_manager::login_state_t::waiting) {
      this->login_state_ = user_manager::login_state_t::login_failed;
    }

  std::set<const_data_buffer> new_channels;
  for (auto & id : new_records) {
    GET_EXPECTED_VALUE(st, t.get_reader(
      t1.select(t1.data)
      .where(t1.id == id)));

    GET_EXPECTED(st_result, st.execute());
    if (!st_result) {
      return vds::make_unexpected<std::runtime_error>("Invalid program");
    }

    const auto data = t1.data.get(st);
    GET_EXPECTED(block, transactions::transaction_block::create(data));

    CHECK_EXPECTED(block.walk_messages(
      [this](const transactions::root_user_transaction & message)->expected<bool> {
      return this->process_root_user_transaction(message);
    },
      [this](const transactions::create_user_transaction & message)->expected<bool> {
      return this->process_create_user_transaction(message);
    },
      [this, &new_channels, tp = block.time_point()](const transactions::channel_message  & message)->expected<bool>{
      return this->process_channel_message(message, new_channels, tp);
    }
    ));
  }
  
  return expected<void>();
}

void vds::_user_manager::add_certificate(const std::shared_ptr<vds::certificate> &cert) {
	this->certificate_chain_[cert->subject()] = cert;
}

vds::member_user vds::_user_manager::get_current_user() const {
  return member_user(this->user_cert_, this->user_private_key_);
}

vds::async_task<vds::expected<bool>> vds::_user_manager::approve_join_request(const const_data_buffer& data) {
  const_data_buffer user_public_key_der;
  std::string user_object_id;
  const_data_buffer user_private_key_der;
  std::string userName;
  std::string userEmail;

  binary_deserializer s(data);
  CHECK_EXPECTED_ASYNC(s >> userName);
  CHECK_EXPECTED_ASYNC(s >> userEmail);
  CHECK_EXPECTED_ASYNC(s >> user_public_key_der);
  CHECK_EXPECTED_ASYNC(s >> user_object_id);
  CHECK_EXPECTED_ASYNC(s >> user_private_key_der);

  const auto pos = s.size();

  const_data_buffer signature;
  CHECK_EXPECTED_ASYNC(s >> signature);

  GET_EXPECTED_ASYNC(user_public_key, asymmetric_public_key::parse_der(user_public_key_der));

  GET_EXPECTED_ASYNC(verify_result, asymmetric_sign_verify::verify(
    hash::sha256(),
    user_public_key,
    signature,
    data.data(),
    data.size() - pos));

  if (!verify_result) {
    co_return make_unexpected<std::runtime_error>("Signature error");
  }

  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
    [
      pthis = this->shared_from_this(),
      user_public_key_param = std::make_shared<asymmetric_public_key>(std::move(user_public_key)),
      user_object_id,
      user_private_key_der,
      userName,
      userEmail
    ](database_transaction & t) -> expected<void> {
    certificate::create_options local_user_options;
    local_user_options.country = "RU";
    local_user_options.organization = "IVySoft";
    local_user_options.name = userName;
    local_user_options.ca_certificate = pthis->user_cert_.get();
    local_user_options.ca_certificate_private_key = pthis->user_private_key_.get();

    GET_EXPECTED(user_cert, certificate::create_new(*user_public_key_param, asymmetric_private_key(), local_user_options));

    GET_EXPECTED(playback, transactions::transaction_block_builder::create(pthis->sp_, t));

    CHECK_EXPECTED(playback.add(
      message_create<transactions::create_user_transaction>(
        user_object_id,
        std::make_shared<certificate>(std::move(user_cert)),
        user_private_key_der,
        userEmail,
        pthis->user_cert_->subject())));

    //auto channel_id = dht::dht_object_id::generate_random_id();

    //auto read_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
    //auto write_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
    //auto channel = member_user(pthis->user_cert_, pthis->user_private_key_).create_channel(
    //  playback,
    //  userName);

    //channel->add_writer(
    //  playback,
    //  pthis->user_name_,
    //  member_user(user_cert, std::shared_ptr<asymmetric_private_key>()),
    //  member_user(pthis->user_cert_, pthis->user_private_key_));
    CHECK_EXPECTED(playback.save(
      pthis->sp_,
      t,
      pthis->user_cert_,
      pthis->user_private_key_));

    return expected<void>();
  }));

  co_return true;
}

const std::string& vds::_user_manager::user_name() const {
  return this->user_name_;
}

vds::expected<bool> vds::_user_manager::parse_join_request(

  const vds::const_data_buffer &data,
  std::string & userName,
  std::string & userEmail) {

  const_data_buffer user_public_key_der;
  std::string user_object_id;
  const_data_buffer user_private_key_der;

  binary_deserializer s(data);
  CHECK_EXPECTED(s >> userName);
  CHECK_EXPECTED(s >> userEmail);
  CHECK_EXPECTED(s >> user_public_key_der);
  CHECK_EXPECTED(s >> user_object_id);
  CHECK_EXPECTED(s >> user_private_key_der);

  auto pos = s.size();

  const_data_buffer signature;
  CHECK_EXPECTED(s >> signature);

  GET_EXPECTED(user_public_key, asymmetric_public_key::parse_der(user_public_key_der));

  return asymmetric_sign_verify::verify(
    hash::sha256(),
    user_public_key,
    signature,
    data.data(),
    data.size() - pos);
}

vds::async_task<vds::expected<vds::user_channel>> vds::_user_manager::create_channel(
  const std::string & channel_type,
  const std::string& name) {
  auto result = std::make_shared<vds::user_channel>();
  CHECK_EXPECTED_ASYNC(co_await this->sp_->get<db_model>()->async_transaction(
    [pthis = this->shared_from_this(), channel_type, name, result](database_transaction & t)->expected<void> {

    GET_EXPECTED(log, vds::transactions::transaction_block_builder::create(pthis->sp_, t));

    vds::asymmetric_private_key channel_read_private_key;
    vds::asymmetric_private_key channel_write_private_key;
    GET_EXPECTED_VALUE(*result, member_user(pthis->user_cert_, pthis->user_private_key_).create_channel(
      log,
      channel_type,
      name));

    CHECK_EXPECTED(log.save(
      pthis->sp_,
      t,
      pthis->user_cert(),
      pthis->user_private_key()));

    CHECK_EXPECTED(pthis->update(t));

    return expected<void>();
  }));

  co_return  std::move(*result);
}

const std::string& vds::user_manager::user_name() const {
  return this->impl_->user_name();
}
