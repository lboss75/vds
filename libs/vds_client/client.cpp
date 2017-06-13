/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client.h"
#include "client_p.h"
#include "client_connection.h"
#include "deflate.h"
#include "file_manager_p.h"

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
    .then([this, user_password](
      const std::function<void(
        const service_provider & sp,
        const certificate & /*server_certificate*/,
        const asymmetric_private_key & /*private_key*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const client_messages::certificate_and_key_response & response) {

    sp.get<logger>()->trace(sp, "Register new server");

    asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
    private_key.generate();

    asymmetric_public_key pkey(private_key);

    auto user_certificate = certificate::parse(response.certificate_body());
    auto user_private_key = asymmetric_private_key::parse(response.private_key_body(), user_password);


    auto server_id = guid::new_guid();
    certificate::create_options options;
    options.country = "RU";
    options.organization = "IVySoft";
    options.name = "Certificate " + server_id.str();
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

    hash ph(hash::sha256());
    ph.update(user_password.c_str(), user_password.length());
    ph.final();

    this->owner_->logic_->send_request<client_messages::register_server_response>(
      sp,
      client_messages::register_server_request(
        server_id,
        response.id(),
        server_certificate.str(),
        private_key.str(user_password),
        ph.signature()).serialize())
      .then([this](const std::function<void(const service_provider & sp)> & done,
        const error_handler & on_error,
        const service_provider & sp,
        const client_messages::register_server_response & response) {
      done(sp);
    }).wait(
      [server_cert = server_certificate.str(), private_key, done](const service_provider & sp) { done(sp, certificate::parse(server_cert), private_key); },
      [on_error, root_folder](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {

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
    .then([this, user_login, name, fn, user_password](
      const std::function<void(const service_provider & sp, const std::string & /*version_id*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const client_messages::certificate_and_key_response & response) {
      
      imt_service::disable_async(sp);

      sp.get<logger>()->trace(sp, "Crypting data");
      auto length = file::length(fn);
      
      //Generate key
      symmetric_key transaction_key(symmetric_crypto::aes_256_cbc());
      transaction_key.generate();

      //Crypt body
      auto version_id = guid::new_guid();
      filename tmp_file(this->tmp_folder_, version_id.str());

      _file_manager::crypt_file(
        sp,
        length,
        transaction_key,
        fn,
        tmp_file)
      .then([this, name, length, version_id, &response, transaction_key, user_password, tmp_file](
        const std::function<void(const service_provider & sp)>& done,
        const error_handler & on_error,
        const service_provider & sp,
        size_t body_size,
        size_t tail_size){
        //Meta info
        binary_serializer file_info;
        file_info << name << length << body_size << tail_size;
        transaction_key.serialize(file_info);
        
        auto user_certificate = certificate::parse(response.certificate_body());

        auto meta_info = user_certificate.public_key().encrypt(file_info.data());
        
        //Form principal_log message
        auto msg = principal_log_record(
          guid::new_guid(),
          response.id(),
          response.active_records(),
          principal_log_new_object(version_id,  body_size + tail_size, meta_info).serialize(),
          response.order_num() + 1).serialize(false);                              
        
        auto s = msg->str();
        const_data_buffer signature;
        dataflow(
          dataflow_arguments<uint8_t>((const uint8_t *)s.c_str(), s.length()),
          asymmetric_sign(
            hash::sha256(),
            asymmetric_private_key::parse(response.private_key_body(), user_password),
            signature)
        )(
          [this, done, on_error, &signature, &response, msg, tmp_file](const service_provider & sp){
            sp.get<logger>()->trace(sp, "Message [%s] signed [%s]", msg->str().c_str(), base64::from_bytes(signature).c_str());

            sp.get<logger>()->trace(sp, "Uploading file");
            
            imt_service::enable_async(sp);            
            this->owner_->logic_->send_request<client_messages::put_object_message_response>(
              sp,
              client_messages::put_object_message(
                response.id(),
                msg,
                signature,
                tmp_file).serialize())
            .then([](
              const std::function<void(const service_provider & /*sp*/)> & done,
              const error_handler & on_error,
              const service_provider & sp,
              const client_messages::put_object_message_response & response) {
                done(sp);
            })
            .wait(done, on_error, sp);
          },
          [on_error, tmp_file](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
            file::delete_file(tmp_file);
            on_error(sp, ex);
          },
          sp);
      }).wait([done, version_id]
      (const service_provider & sp){
        done(sp, version_id.str());
      },
      on_error, sp);
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
        const client_messages::certificate_and_key_response & response) {

    //sp.get<logger>()->trace(sp, "Downloading file");
    //this->owner_->logic_->download_file(sp, user_login, name).wait(
    //  [this, done, on_error, user_certificate = certificate::parse(user_certificate.str()), user_private_key, target_file]
    //  (const service_provider & sp, const const_data_buffer & meta_info, const filename & tmp_file) {

    //  sp.get<logger>()->trace(sp, "Decrypting data");
    //  const_data_buffer key_crypted;
    //  const_data_buffer signature;

    //  binary_deserializer datagram(meta_info);
    //  datagram
    //    >> key_crypted
    //    >> signature;

    //  binary_serializer to_sign;
    //  to_sign << key_crypted;

    //  if (!asymmetric_sign_verify::verify(
    //    hash::sha256(),
    //    user_certificate.public_key(),
    //    signature,
    //    to_sign.data().data(),
    //    to_sign.data().size())) {
    //    throw std::runtime_error("Invalid data");
    //  }

    //  symmetric_key transaction_key(
    //    symmetric_crypto::aes_256_cbc(),
    //    binary_deserializer(user_private_key.decrypt(key_crypted)));

    //  const std::shared_ptr<std::exception> & error;
    //  dataflow(
    //    file_read(tmp_file),
    //    symmetric_decrypt(transaction_key),
    //    deflate(),
    //    file_write(target_file, file::file_mode::create_new))(
    //      [](const service_provider & sp) { },
    //      [&error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) { error = ex; },
    //      sp);

    //  if (error) {
    //    on_error(sp, error);
    //  }
    //  else {
    //    done(sp);
    //  }
    //},
    //  on_error,
    //  sp);
  });
}

vds::async_task<const vds::client_messages::certificate_and_key_response & /*response*/>
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
      user_login,
      ph.signature()).serialize())
    .then([user_password](const std::function<void(
      const service_provider & /*sp*/,
      const client_messages::certificate_and_key_response & /*response*/)> & done,
      const error_handler & on_error,
      const service_provider & sp,
      const client_messages::certificate_and_key_response & response) {
    done(sp, response);
  });
}

