/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_manager.h"
#include "member_user.h"
#include "private/member_user_p.h"
#include "iserver_api.h"

std::shared_ptr<vds::user_channel> vds::user_manager::get_channel(const const_data_buffer& channel_id) const
{
  auto p = this->channels_.find(channel_id);
  if (this->channels_.end() != p) {
    return p->second;
  }

  return std::shared_ptr<user_channel>();
}

vds::async_task<vds::expected<void>> vds::user_manager::reset(
    iserver_api & client,
    const std::string &root_user_name,
    const std::string &root_password,
    const keys_control::private_info_t& private_info) {

  transactions::transaction_block_builder playback;

    //Create root user
    GET_EXPECTED_ASYNC(root_user, co_await _member_user::create_user(
      client,
      playback,
      root_user_name,
      root_user_name,
      root_password,
      private_info.root_private_key_));

    //common news
    GET_EXPECTED_ASYNC(pc, root_user->personal_channel());
    CHECK_EXPECTED_ASYNC(pc.add_log(
      playback,
      message_create<transactions::channel_create_transaction>(
        keys_control::get_common_news_channel_id(),
        user_channel::channel_type_t::news_channel,
        "Common news",
        keys_control::get_common_news_read_public_key(),
        keys_control::get_common_news_read_private_key(),
        keys_control::get_common_news_write_public_key(),
        private_info.common_news_write_private_key_)));

    //Auto update
    CHECK_EXPECTED_ASYNC(pc.add_log(
      playback,
      message_create<transactions::channel_create_transaction>(
        keys_control::get_autoupdate_channel_id(),
        user_channel::channel_type_t::file_channel,
        "Auto update",
        keys_control::get_autoupdate_read_public_key(),
        keys_control::get_autoupdate_read_private_key(),
        keys_control::get_autoupdate_write_public_key(),
        private_info.autoupdate_write_private_key_)));

    //Create auto update user
    GET_EXPECTED_ASYNC(autoupdate_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));

    GET_EXPECTED_ASYNC(
      auto_update_user,
      co_await root_user->create_user(
        client,
        playback,
        "Auto Update",
        keys_control::auto_update_login(),
        keys_control::auto_update_password(),
        std::make_shared<asymmetric_private_key>(std::move(autoupdate_private_key))));

    GET_EXPECTED_ASYNC(auto_update_user_personal_channel, auto_update_user->personal_channel());
    CHECK_EXPECTED_ASYNC(auto_update_user_personal_channel.add_log(
      playback,
      message_create<transactions::channel_add_reader_transaction>(
        keys_control::get_autoupdate_channel_id(),
        user_channel::channel_type_t::file_channel,
        "Auto update",
        keys_control::get_autoupdate_read_public_key(),
        keys_control::get_autoupdate_read_private_key(),
        keys_control::get_autoupdate_write_public_key())));

    //Web
    CHECK_EXPECTED_ASYNC(pc.add_log(
      playback,
      message_create<transactions::channel_create_transaction>(
        keys_control::get_web_channel_id(),
        user_channel::channel_type_t::file_channel,
        "Web",
        keys_control::get_web_read_public_key(),
        keys_control::get_web_read_private_key(),
        keys_control::get_web_write_public_key(),
        private_info.web_write_private_key_)));

    //Create web user
    GET_EXPECTED_ASYNC(web_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
    GET_EXPECTED_ASYNC(
      web_user,
      co_await root_user->create_user(
        client,
        playback,
        "Web",
        keys_control::web_login(),
        keys_control::web_password(),
        std::make_shared<asymmetric_private_key>(std::move(web_private_key))));

    GET_EXPECTED_ASYNC(web_user_personal_channel, web_user->personal_channel());
    CHECK_EXPECTED_ASYNC(web_user_personal_channel.add_log(
      playback,
      message_create<transactions::channel_add_reader_transaction>(
        keys_control::get_web_channel_id(),
        user_channel::channel_type_t::file_channel,
        "Web",
        keys_control::get_web_read_public_key(),
        keys_control::get_web_read_private_key(),
        keys_control::get_web_write_public_key())));

    CHECK_EXPECTED_ASYNC(co_await client.broadcast(playback.close()));
    co_return expected<void>();
}
/*
std::shared_ptr<vds::user_channel> vds::user_manager::get_channel(
  const const_data_buffer & channel_id) const
{
  return this->impl_->get_channel(channel_id);
}

std::map<vds::const_data_buffer, std::shared_ptr<vds::user_channel>> vds::user_manager::get_channels() const {
  std::list<vds::user_channel> result;

  return this->impl_->channels();
}

vds::expected<uint64_t> vds::user_manager::get_device_storage_used() {
  GET_EXPECTED(owner_id, this->get_current_user().user_public_key()->fingerprint());

  GET_EXPECTED(result, user_storage::device_storage(
    this->sp_).get());

  return result.used_size;
}

vds::expected<uint64_t> vds::user_manager::get_device_storage_size() {
  GET_EXPECTED(owner_id, this->get_current_user().user_public_key()->fingerprint());

  GET_EXPECTED(result, user_storage::device_storage(
    this->sp_).get());

  return result.reserved_size;
}

vds::expected<uint64_t> vds::user_manager::get_user_balance() {
  return 0;
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
//vds::expected<void> vds::user_manager::save_certificate(
//    vds::database_transaction &t,
//    const vds::asymmetric_public_key &cert) {
//
//  orm::certificate_chain_dbo t1;
//  GET_EXPECTED(st, t.get_reader(t1.select(t1.id).where(t1.id == cert.subject())));
//  GET_EXPECTED(st_result, st.execute());
//  if (!st_result) {
//    GET_EXPECTED(der, cert.der());
//    CHECK_EXPECTED(t.execute(t1.insert(
//      t1.id = cert.subject(),
//      t1.cert = der,
//      t1.parent = cert.issuer())));
//  }
//
//  orm::certificate_unknown_dbo t2;
//  return t.execute(t2.delete_if(t2.id == cert.subject()));
//}

vds::member_user vds::user_manager::get_current_user() const {
  return this->impl_->get_current_user();
}

const std::shared_ptr<vds::asymmetric_private_key> & vds::user_manager::get_current_user_private_key() const {
  return this->impl_->get_current_user_private_key();
}

vds::async_task<vds::expected<void>> vds::user_manager::create_user(
  const service_provider * sp,
  const std::string& userEmail,
  const std::string& userPassword) {

  return sp->get<db_model>()->async_transaction(
    [
      sp,
      userEmail,
      userPassword
    ](database_transaction & t)->expected<void> {

    GET_EXPECTED(user_id, dht::dht_object_id::user_credentials_to_key(userPassword));
    GET_EXPECTED(user_private_key, vds::asymmetric_private_key::generate(
      vds::asymmetric_crypto::rsa4096()));
    GET_EXPECTED(user_private_key_der, user_private_key.der(userPassword));

    GET_EXPECTED(user_public_key, asymmetric_public_key::create(user_private_key));

    GET_EXPECTED(playback, transactions::transaction_block_builder::create(sp, t));

    CHECK_EXPECTED(playback.add(
      message_create<transactions::create_user_transaction>(
        user_id,
        std::make_shared<asymmetric_public_key>(std::move(user_public_key)),
        user_private_key_der,
        userEmail)));

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
    auto client = sp->get<dht::network::client>();
    CHECK_EXPECTED(client->save(sp, playback, t));

    return expected<void>();
  });
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
  GET_EXPECTED_VALUE(this->user_credentials_key_, dht::dht_object_id::user_credentials_to_key(user_password));
  this->user_name_ = user_login;
  this->user_password_ = user_password;
  return expected<void>();
}

vds::expected<bool> vds::_user_manager::process_create_user_transaction(
  const transactions::create_user_transaction & message) {
  if (this->user_credentials_key_ == message.user_credentials_key && this->user_name_ == message.user_name) {
    this->user_cert_ = message.user_public_key;

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
    auto channel_read_key = channel->read_cert_private_key(message.read_id());
    if (channel_read_key) {
      CHECK_EXPECTED(message.walk_messages(this->sp_, *channel_read_key, transactions::message_environment_t{ tp, "???" },
        [this, channel_id = message.channel_id(), log](
          const transactions::channel_add_reader_transaction & message,
          const transactions::message_environment_t & / *message_environment* /)->expected<bool> {
        GET_EXPECTED(read_id, message.read_public_key->fingerprint());
        GET_EXPECTED(write_id, message.write_public_key->fingerprint());
        auto cp = std::make_shared<user_channel>(
          message.id,
          message.channel_type,
          message.name,
          read_id,
          message.read_public_key,
          message.read_private_key,
          write_id,
          message.write_public_key,
          std::shared_ptr<asymmetric_private_key>());

        this->channels_[cp->id()] = cp;
        log->debug(ThisModule, "Got channel %s reader public key",
          base64::from_bytes(cp->id()).c_str());

        return true;
      },
        [this, channel_id = message.channel_id(), log](
          const transactions::channel_add_writer_transaction & message,
          const transactions::message_environment_t & /*message_environment* /)->expected<bool> {
        GET_EXPECTED(read_id, message.read_public_key->fingerprint());
        GET_EXPECTED(write_id, message.write_public_key->fingerprint());
        auto cp = std::make_shared<user_channel>(
          message.id,
          message.channel_type,
          message.name,
          read_id,
          message.read_public_key,
          message.read_private_key,
          write_id,
          message.write_public_key,
          message.write_private_key);

        this->channels_[cp->id()] = cp;

        log->debug(ThisModule, "Got channel %s write public key",
          base64::from_bytes(cp->id()).c_str());

        return true;
      },
        [this, channel_id = message.channel_id(), log, &new_channels](
          const transactions::channel_create_transaction & message,
          const transactions::message_environment_t & /*message_environment* /)->expected<bool>{
        if (new_channels.end() == new_channels.find(channel_id)) {
          new_channels.emplace(message.channel_id);
        }
        GET_EXPECTED(read_id, message.read_public_key->fingerprint());
        GET_EXPECTED(write_id, message.write_public_key->fingerprint());
        auto cp = std::make_shared<user_channel>(
          message.channel_id,
          message.channel_type,
          message.name,
          read_id,
          message.read_public_key,
          message.read_private_key,
          write_id,
          message.write_public_key,
          message.write_private_key);

        this->channels_[cp->id()] = cp;

        return true;
      },
        [this, channel_id = message.channel_id(), log](
          const transactions::control_message_transaction & message,
          const transactions::message_environment_t & /*message_environment* /)->expected<bool> {
        auto msg = std::dynamic_pointer_cast<json_object>(message.message);
        std::string type;
        if (msg) {
          GET_EXPECTED(have_type, msg->get_property("$type", type));
          if (have_type) {
            if (transactions::control_message_transaction::create_wallet_type == type) {
              std::string name;
              CHECK_EXPECTED(msg->get_property("name", name));

              GET_EXPECTED(public_key, asymmetric_public_key::parse_der(message.attachments.at("public_key")));
              GET_EXPECTED(private_key, asymmetric_private_key::parse_der(message.attachments.at("key"), std::string()));
              auto wallet = std::make_shared<user_wallet>(
                name,
                std::move(public_key),
                std::move(private_key));

              this->wallets_.push_back(wallet);

              log->debug(ThisModule, "Got wallet %s write public key",
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

    //if (new_records.empty() && this->login_state_ == user_manager::login_state_t::waiting) {
    //  this->login_state_ = user_manager::login_state_t::login_failed;
    //}

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
      [this](const transactions::create_user_transaction & message)->expected<bool> {
        return this->process_create_user_transaction(message);
      },
      [this, &new_channels, tp = block.time_point()](const transactions::channel_message  & message)->expected<bool>{
        return this->process_channel_message(message, new_channels, tp);
      },
      [this, &block](const transactions::store_block_transaction & message)->expected<bool> {
        if (!this->user_cert_) {
          return expected<bool>(true);
        }

        GET_EXPECTED(user_id, this->user_cert_->fingerprint());

        if (message.owner_id != user_id) {
          return expected<bool>(true);
        }

        CHECK_EXPECTED(this->mutual_settlements_->update(block, message));
        return expected<bool>(true);
      },
      [this, &block](const transactions::payment_transaction & message)->expected<bool> {
        if (message.payment_type != "mutual_settlements") {
          return expected<bool>(true);
        }

        bool my_wallet = false;
        for (const auto & wallet : this->wallets()) {
          GET_EXPECTED(wallet_id, wallet->public_key().fingerprint());
          if (wallet_id == message.source_wallet) {
            my_wallet = true;
          }
        }

        if (!my_wallet) {
          return expected<bool>(true);
        }

        CHECK_EXPECTED(this->mutual_settlements_->update(block, message));
        return expected<bool>(true);
      }
    ));
  }

  if (new_records.empty()) {
    CHECK_EXPECTED(this->mutual_settlements_->calculate(this->sp_, this->wallets(), t));
  }
  
  return expected<void>();
}

vds::expected<void> vds::_user_manager::add_public_key(const std::shared_ptr<vds::asymmetric_public_key> &public_key) {
  GET_EXPECTED(id, public_key->fingerprint());
	this->certificate_chain_[id] = public_key;
  return expected<void>();
}

vds::member_user vds::_user_manager::get_current_user() const {
  return member_user(this->user_cert_, this->user_private_key_);
}

const std::string& vds::_user_manager::user_name() const {
  return this->user_name_;
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

    CHECK_EXPECTED(
      pthis->sp_->get<dht::network::client>()->save(
        pthis->sp_,
        log,
        t));

    CHECK_EXPECTED(pthis->update(t));

    return expected<void>();
  }));

  co_return  std::move(*result);
}

const std::string& vds::user_manager::user_name() const {
  return this->impl_->user_name();
}
*/