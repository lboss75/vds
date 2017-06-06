/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client.h"
#include "client_p.h"
#include "client_connection.h"
#include "deflate.h"

vds::client::client(const std::string & server_address)
: server_address_(server_address),
  impl_(new _client(this)),
  logic_(new client_logic())
{
}

vds::client::~client()
{
}

void vds::client::register_services(service_registrator & registrator)
{
  registrator.add_service<iclient>(this->impl_.get());
}

void vds::client::start(const service_provider & sp)
{
  this->impl_->start(sp);
  //Load certificates
  certificate * client_certificate = nullptr;
  asymmetric_private_key * client_private_key = nullptr;

  filename cert_name(foldername(persistence::current_user(sp), ".vds"), "client.crt");
  filename pkey_name(foldername(persistence::current_user(sp), ".vds"), "client.pkey");

  if (file::exists(cert_name)) {
    this->client_certificate_.load(cert_name);
    this->client_private_key_.load(pkey_name);

    client_certificate = &this->client_certificate_;
    client_private_key = &this->client_private_key_;
  }

  this->logic_->start(sp, this->server_address_, client_certificate, client_private_key);

}

void vds::client::stop(const service_provider & sp)
{
  this->logic_->stop(sp);
  this->impl_->stop(sp);
}

void vds::client::connection_closed()
{
}

void vds::client::connection_error()
{
}

vds::async_task<
  const vds::certificate & /*server_certificate*/,
  const vds::asymmetric_private_key & /*private_key*/>
  vds::iclient::init_server(
  const service_provider & sp,
  const std::string & user_login,
  const std::string & user_password)
{
  return static_cast<_client *>(this)->init_server(sp, user_login, user_password);
}

vds::async_task<const std::string& /*version_id*/> vds::iclient::upload_file(
  const service_provider & sp,
  const std::string & login,
  const std::string & password,
  const std::string & name,
  const filename & tmp_file)
{
  return static_cast<_client *>(this)->upload_file(sp, login, password, name, tmp_file);
}

vds::async_task<> vds::iclient::download_data(
  const service_provider & sp,
  const std::string & login,
  const std::string & password,
  const std::string & name,
  const filename & target_file)
{
  return static_cast<_client *>(this)->download_data(sp, login, password, name, target_file);
}

vds::_client::_client(vds::client* owner)
: owner_(owner), last_tmp_file_index_(0)
{
}

void vds::_client::start(const service_provider & sp)
{
  this->tmp_folder_ = foldername(foldername(persistence::current_user(sp), ".vds"), "tmp");
  this->tmp_folder_.create();
}

void vds::_client::stop(const service_provider & sp)
{
}


vds::async_task<
  const vds::certificate & /*server_certificate*/,
  const vds::asymmetric_private_key & /*private_key*/>
vds::_client::init_server(
  const service_provider & sp,
  const std::string& user_login,
  const std::string& user_password)
{
  return
    this->authenticate(sp, user_login, user_password)
    .then([this](
      const std::function<void(
        const service_provider & sp,
        const certificate & /*server_certificate*/,
        const asymmetric_private_key & /*private_key*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const certificate& user_certificate,
      const asymmetric_private_key& user_private_key) {

    sp.get<logger>()->trace(sp, "Register new server");

    asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
    private_key.generate();

    asymmetric_public_key pkey(private_key);

    certificate::create_options options;
    options.country = "RU";
    options.organization = "IVySoft";
    options.name = "Certificate " + guid::new_guid().str();
    options.ca_certificate = &user_certificate;
    options.ca_certificate_private_key = &user_private_key;

    certificate server_certificate = certificate::create_new(pkey, private_key, options);
    foldername root_folder(persistence::current_user(sp), ".vds");
    root_folder.create();
    
    sp.get<logger>()->trace(sp, "Register new user");
    asymmetric_private_key local_user_private_key(asymmetric_crypto::rsa4096());
    local_user_private_key.generate();

    asymmetric_public_key local_user_pkey(local_user_private_key);

    certificate::create_options local_user_options;
    local_user_options.country = "RU";
    local_user_options.organization = "IVySoft";
    local_user_options.name = "Local User Certificate";
    local_user_options.ca_certificate = &user_certificate;
    local_user_options.ca_certificate_private_key = &user_private_key;

    certificate local_user_certificate = certificate::create_new(local_user_pkey, local_user_private_key, local_user_options);

    user_certificate.save(filename(root_folder, "owner.crt"));
    local_user_certificate.save(filename(root_folder, "user.crt"));
    local_user_private_key.save(filename(root_folder, "user.pkey"));

    this->owner_->logic_->send_request<client_messages::register_server_response>(
      sp,
      client_messages::register_server_request(server_certificate.str()).serialize())
      .then([this](const std::function<void(const service_provider & sp)> & done,
        const error_handler & on_error,
        const service_provider & sp,
        const client_messages::register_server_response & response) {
      done(sp);
    }).wait(
      [server_cert = server_certificate.str(), private_key, done](const service_provider & sp) { done(sp, certificate::parse(server_cert), private_key); },
      [on_error, root_folder](const service_provider & sp, std::exception_ptr ex) {

        file::delete_file(filename(root_folder, "user.crt"), true);
        file::delete_file(filename(root_folder, "user.pkey"), true);

        on_error(sp, ex);
      },
      sp);
  });
}

vds::async_task<const std::string& /*version_id*/> vds::_client::upload_file(
  const service_provider & sp,
  const std::string & user_login,
  const std::string & user_password,
  const std::string & name,
  const filename & fn)
{
  return
    this->authenticate(sp, user_login, user_password)
    .then([this, user_login, name, fn](
      const std::function<void(const service_provider & sp, const std::string& /*version_id*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const certificate& user_certificate,
      const asymmetric_private_key& user_private_key) {

    sp.get<logger>()->trace(sp, "Crypting data");

    symmetric_key transaction_key(symmetric_crypto::aes_256_cbc());
    transaction_key.generate();

    binary_serializer key_data;
    transaction_key.serialize(key_data);

    auto key_crypted = user_certificate.public_key().encrypt(key_data.data());

    this->tmp_folder_mutex_.lock();
    auto tmp_index = this->last_tmp_file_index_++;
    this->tmp_folder_mutex_.unlock();

    filename tmp_file(this->tmp_folder_, std::to_string(tmp_index));

    binary_serializer to_sign;
    to_sign << key_crypted;

    binary_serializer datagram;
    datagram << key_crypted;
    datagram << asymmetric_sign::signature(
      hash::sha256(),
      user_private_key,
      to_sign.data());

    dataflow(
      file_read(fn),
      deflate(),
      symmetric_encrypt(transaction_key),
      file_write(tmp_file, file::file_mode::create_new))(
        [this, tmp_file, user_login, name, done, on_error, info = const_data_buffer(datagram.data())](const service_provider & sp) {

      sp.get<logger>()->trace(sp, "Upload file");
      this->owner_->logic_->put_file(
        sp,
        user_login,
        name,
        info,
        tmp_file)
        .wait(done, on_error, sp);
    },
        [](const service_provider & sp, std::exception_ptr ex) { std::rethrow_exception(ex); },
      sp);

  });
}

vds::async_task<>
vds::_client::download_data(
  const service_provider & sp,
  const std::string & user_login,
  const std::string & user_password,
  const std::string & name,
  const filename & target_file)
{
  return this->authenticate(
    sp,
    user_login,
    user_password)
    .then(
      [this, user_login, name, target_file](
        const std::function<void(const service_provider & sp)>& done,
        const error_handler & on_error,
        const service_provider & sp,
        const certificate& user_certificate,
        const asymmetric_private_key& user_private_key) {

    sp.get<logger>()->trace(sp, "Downloading file");
    this->owner_->logic_->download_file(sp, user_login, name).wait(
      [this, done, on_error, user_certificate = certificate::parse(user_certificate.str()), user_private_key, target_file]
      (const service_provider & sp, const const_data_buffer & meta_info, const filename & tmp_file) {

      sp.get<logger>()->trace(sp, "Decrypting data");
      const_data_buffer key_crypted;
      const_data_buffer signature;

      binary_deserializer datagram(meta_info);
      datagram
        >> key_crypted
        >> signature;

      binary_serializer to_sign;
      to_sign << key_crypted;

      if (!asymmetric_sign_verify::verify(
        hash::sha256(),
        user_certificate.public_key(),
        signature,
        to_sign.data().data(),
        to_sign.data().size())) {
        throw std::runtime_error("Invalid data");
      }

      symmetric_key transaction_key(
        symmetric_crypto::aes_256_cbc(),
        binary_deserializer(user_private_key.decrypt(key_crypted)));

      std::exception_ptr error;
      dataflow(
        file_read(tmp_file),
        symmetric_decrypt(transaction_key),
        deflate(),
        file_write(target_file, file::file_mode::create_new))(
          [](const service_provider & sp) { },
          [&error](const service_provider & sp, std::exception_ptr ex) { error = ex; },
          sp);

      if (error) {
        on_error(sp, error);
      }
      else {
        done(sp);
      }
    },
      on_error,
      sp);
  });
}

vds::async_task<
  const vds::certificate & /*user_certificate*/,
  const vds::asymmetric_private_key & /*user_private_key*/>
  vds::_client::authenticate(
    const service_provider & sp,
    const std::string & user_login,
    const std::string & user_password)
{
  sp.get<logger>()->trace(sp, "Authenticating user %s", user_login.c_str());

  hash ph(hash::sha256());
  ph.update(user_password.c_str(), user_password.length());
  ph.final();

  return this->owner_->logic_->send_request<client_messages::certificate_and_key_response>(
    sp,
    client_messages::certificate_and_key_request(
      "login:" + user_login,
      ph.signature()).serialize())
    .then([user_password](const std::function<void(
      const service_provider & /*sp*/,
      const certificate & /*user_certificate*/,
      const asymmetric_private_key & /*user_private_key*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const client_messages::certificate_and_key_response & response) {
    done(
      sp,
      certificate::parse(response.certificate_body()),
      asymmetric_private_key::parse(response.private_key_body(), user_password));
  });
}

