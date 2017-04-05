/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client.h"
#include "client_connection.h"

vds::client::client()
{
}

vds::client::~client()
{
}

void vds::client::register_services(service_registrator & registrator)
{
  registrator.add_factory<iclient>([this](const service_provider & sp, bool & is_scoped)->iclient {
    return iclient(sp, this);
  });
}

void vds::client::start(const service_provider & sp)
{
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

  this->logic_.reset(new client_logic(sp, client_certificate, client_private_key));
  this->logic_->start();
}

void vds::client::stop(const service_provider & sp)
{
  this->logic_->stop();
}

void vds::client::connection_closed()
{
}

void vds::client::connection_error()
{
}


vds::iclient::iclient(const service_provider & sp, vds::client* owner)
  : sp_(sp), log_(sp, "Client"), owner_(owner)
{
}

vds::async_task<>
vds::iclient::init_server(
  const std::string& user_login,
  const std::string& user_password)
{
  return 
    this->authenticate(user_login, user_password)
    .then([this](
      const std::function<void(void)> & done,
      const error_handler & on_error,
      const certificate& user_certificate,
      const asymmetric_private_key& user_private_key){

        this->log_(ll_trace, "Register new server");

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

        auto request_id = guid::new_guid().str();
        client_messages::register_server_request register_message(
          request_id,
          server_certificate.str());

        json_writer register_message_writer;
        register_message.serialize()->str(register_message_writer);

        std::string error;
        if (!this->owner_->logic_->add_task_and_wait<client_messages::register_server_response>(
          register_message_writer.str(),
          [&request_id, &error](const client_messages::register_server_response & message) -> bool {
          if (request_id != message.request_id()) {
            return false;
          }

          error = message.error();
          return true;
        })) {
          throw new std::runtime_error("Timeout at registering new server");
        }

        if (!error.empty()) {
          throw new std::runtime_error(error);
        }

        foldername root_folder(persistence::current_user(this->sp_), ".vds");
        root_folder.create();

        server_certificate.save(filename(root_folder, "server.crt"));
        private_key.save(filename(root_folder, "server.pkey"));


        this->log_(ll_trace, "Register new user");
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
    });
}

vds::async_task<> vds::iclient::upload_file(
  const std::string & user_login,
  const std::string & user_password,
  const std::string & name,
  const void * data,
  size_t data_size)
{
  return 
    this->authenticate(user_login, user_password)
    .then([this, user_login, data, data_size](
      const std::function<void(void)> & done,
      const error_handler & on_error,
      const certificate& user_certificate,
      const asymmetric_private_key& user_private_key){

  this->log_(ll_trace, "Crypting data");

  symmetric_key transaction_key(symmetric_crypto::aes_256_cbc());
  transaction_key.generate();

  binary_serializer key_data;
  transaction_key.serialize(key_data);

  auto key_crypted = user_certificate.public_key().encrypt(key_data.data());

  dataflow(
    symmetric_encrypt(this->sp_, transaction_key),
    collect_data())(
      [this, key_crypted, user_private_key, user_login](const void * data, size_t data_size) {

    binary_serializer to_sign;
    to_sign << key_crypted;
    to_sign.push_data(data, data_size);

    binary_serializer datagram;
    datagram << key_crypted;
    datagram.push_data(data, data_size);
    datagram << asymmetric_sign::signature(
      hash::sha256(),
      user_private_key,
      to_sign.data());


    this->log_(ll_trace, "Upload file");
    this->owner_->logic_->put_file(user_login, datagram.data());
  },
      [](std::exception_ptr ex) { std::rethrow_exception(ex); },
    data,
    data_size);

  });
}

vds::async_task<vds::data_buffer &&>
vds::iclient::download_data(
  const std::string & user_login,
  const std::string & user_password,
  const std::string & name)
{
  return this->authenticate(
    user_login,
    user_password)
  .then([this, user_login](
    const std::function<void(vds::data_buffer&&)>& done,
    const error_handler & on_error,
    const certificate& user_certificate,
    const asymmetric_private_key& user_private_key){

    this->log_(ll_trace, "Download file");
    auto datagram_data = this->owner_->logic_->download_file(user_login);

    this->log_(ll_trace, "Decrypting data");
    data_buffer key_crypted;
    data_buffer crypted_data;
    data_buffer signature;

    binary_deserializer datagram(datagram_data);
    datagram
      >> key_crypted
      >> crypted_data
      >> signature;

    binary_serializer to_sign;
    to_sign << key_crypted << crypted_data;

    if (!asymmetric_sign_verify::verify(
      hash::sha256(),
      user_certificate.public_key(),
      signature,
      to_sign.data().data(),
      to_sign.data().size())) {
      throw new std::runtime_error("Invalid data");
    }

    symmetric_key transaction_key(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(user_private_key.decrypt(key_crypted)));

    barrier b;
    data_buffer result;
    dataflow(
      symmetric_decrypt(this->sp_, transaction_key),
      collect_data())(
        [&result, &b](const void * data, size_t size) {result.reset(data, size); b.set(); },
        [](std::exception_ptr ex) { std::rethrow_exception(ex); },
        crypted_data.data(),
        crypted_data.size());

    b.wait();
    done(std::move(result));
  });
}

vds::async_task<
  const vds::certificate & /*user_certificate*/,
  const vds::asymmetric_private_key & /*user_private_key*/>
  
  vds::iclient::authenticate(
  const std::string & user_login,
  const std::string & user_password)
{
  this->log_(ll_trace, "Authenticating user %s", user_login.c_str());
  
  hash ph(hash::sha256());
  ph.update(user_password.c_str(), user_password.length());
  ph.final();

  return this->owner_->logic_->send_request<client_messages::certificate_and_key_response>(
    client_messages::certificate_and_key_request(
      "login:" + user_login,
      ph.signature()).serialize())
  .then([user_password](const std::function<void(
    const certificate & /*user_certificate*/,
    const asymmetric_private_key & /*user_private_key*/)> & done,
    const error_handler & on_error,
    const client_messages::certificate_and_key_response & response){
      done(
        certificate::parse(response.certificate_body()),
        asymmetric_private_key::parse(response.private_key_body(), user_password));
  });
}
