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
  registrator.add_factory<vsr_protocol::iserver_queue>([this](const service_provider &, bool &)->vsr_protocol::iserver_queue{
    return vsr_protocol::iserver_queue(this);
  });
  
  registrator.add_factory<vsr_protocol::iclient>([this](const service_provider &, bool &)->vsr_protocol::iclient{
    return vsr_protocol::iclient(this->vsr_client_protocol_.get());
  });
  
  registrator.add_factory<iclient>([this](const service_provider & sp, bool & is_scoped)->iclient{
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

  if (cert_name.exists()) {
    this->client_certificate_.load(cert_name);
    this->client_private_key_.load(pkey_name);

    client_certificate = &this->client_certificate_;
    client_private_key = &this->client_private_key_;
  }

  this->vsr_client_protocol_.reset(new vsr_protocol::client(sp));
  this->vsr_client_protocol_->start();
  
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


void vds::client::new_client()
{

}

vds::iclient::iclient(const service_provider & sp, vds::client* owner)
: sp_(sp), log_(sp, "Client"), owner_(owner)
{
}

void vds::iclient::init_server(
  const std::string& user_login,
  const std::string& user_password)
{
  this->log_(ll_trace, "Authenticating user %s", user_login.c_str());
  hash ph(hash::sha256());
  ph.update(user_password.c_str(), user_password.length());
  ph.final();

  auto request_id = guid::new_guid_string();
  client_messages::certificate_and_key_request m(request_id, "login:" + user_login, base64::from_bytes(ph.signature(), ph.signature_length()));

  json_writer writer;
  m.serialize()->str(writer);

  this->owner_->logic_->add_task(writer.str());

  std::string error;
  std::string cert_body;
  std::string pkey_body;
  if (!this->owner_->logic_->wait_for(
    std::chrono::seconds(20),
    client_messages::certificate_and_key_response::message_type,
    [&request_id, &error, &cert_body, &pkey_body](const json_object * value) -> bool {
    client_messages::certificate_and_key_response message(value);
    if (request_id != message.request_id()) {
      return false;
    }

    if (!message.error().empty()) {
      error = message.error();
    }
    else {
      cert_body = message.certificate_body();
      pkey_body = message.private_key_body();
    }
    return true;
  })) {
    throw new std::runtime_error("Timeout at getting user certificate");
  }
  
  if (!error.empty()) {
    throw new std::runtime_error(error);
  }

  this->log_(ll_trace, "Register new server");
  certificate user_certificate = certificate::parse(cert_body);
  asymmetric_private_key user_private_key = asymmetric_private_key::parse(pkey_body, user_password);

  asymmetric_private_key private_key(asymmetric_crypto::rsa4096());
  private_key.generate();

  asymmetric_public_key pkey(private_key);

  certificate::create_options options;
  options.country = "RU";
  options.organization = "IVySoft";
  options.name = "Server Certificate";
  options.ca_certificate = &user_certificate;
  options.ca_certificate_private_key = &user_private_key;

  certificate server_certificate = certificate::create_new(pkey, private_key, options);

  request_id = guid::new_guid_string();
  client_messages::register_server_request register_message(
    request_id,
    server_certificate.str());

  json_writer register_message_writer;
  register_message.serialize()->str(register_message_writer);

  this->owner_->logic_->add_task(register_message_writer.str());

  
  if (!this->owner_->logic_->wait_for(
    std::chrono::seconds(20),
    client_messages::register_server_response::message_type,
    [&request_id, &error](const json_object * value) -> bool {
    client_messages::register_server_response message(value);
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

  server_certificate.save(filename(foldername(persistence::current_user(this->sp_), ".vds"), "server.crt"));
  private_key.save(filename(foldername(persistence::current_user(this->sp_), ".vds"), "server.pkey"));
}

