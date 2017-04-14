/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "server_udp_api.h"
#include "server_udp_api_p.h"
#include "crypto_exception.h"
#include "cert_manager.h"

vds::server_udp_api::server_udp_api(
  const service_provider& sp)
: impl_(new _server_udp_api(sp, this))
{
}

vds::server_udp_api::~server_udp_api()
{
  delete this->impl_;
}

void vds::server_udp_api::start(const std::string& address, size_t port)
{
  this->impl_->start(address, port);
}

void vds::server_udp_api::stop()
{
  this->impl_->stop();
}

void vds::server_udp_api::open_udp_session(const std::string & address)
{
  this->impl_->open_udp_session(address);
}

/////////////////////////////////////////////////////////////////
vds::_server_udp_api::_server_udp_api(
  const service_provider & sp,
  server_udp_api * owner)
: sp_(sp),
  log_(sp, "Server UDP API"),
  owner_(owner),
  s_(sp),
  message_queue_(sp)
{
}

vds::_server_udp_api::~_server_udp_api()
{
  //this->sp_.get<iserver>().get_connection_manager().remove_target(this);
}

void vds::_server_udp_api::start(const std::string& address, size_t port)
{
  run_udp_server(this->sp_, this->s_, address, port, *this);
}

void vds::_server_udp_api::stop()
{
  //this->sp_.get<peer_network>().remove_client_channel(this);
}

void vds::_server_udp_api::udp_server_done()
{
}

void vds::_server_udp_api::udp_server_error(std::exception_ptr ex)
{
}

void vds::_server_udp_api::socket_closed(std::list<std::exception_ptr> errors)
{
}

vds::async_task<> vds::_server_udp_api::input_message(const sockaddr_in * from, const void * data, size_t len)
{
  return create_async_task([this, from, data, len](const std::function<void(void)> & done, const error_handler & on_error){
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch(cmd) {
      case udp_messages::message_identification::hello_message_id:
      {
        this->log_.debug("hello from %s", network_service::to_string(*from).c_str());
        udp_messages::hello_message msg(s);
        
        auto server = this->sp_.get<iserver>();
        auto cert_manager = this->sp_.get<vds::cert_manager>();
        
        auto cert = certificate::parse(msg.source_certificate());
        if(cert_manager.validate(cert)){
          symmetric_key session_key(symmetric_crypto::aes_256_cbc());
          session_key.generate();
          
          auto server_id = server_certificate::server_id(cert);
          
          uint32_t session_id;
          for(;;){
            session_id = (uint32_t)std::rand();
            if(this->in_sessions_.end() == this->in_sessions_.find(session_id)){
              break;
            }
          }
          this->in_sessions_[session_id].reset(new in_session_data(server_id, session_key));
          
          binary_serializer s;
          s
            << msg.session_id()
            << session_id;
            
          binary_serializer key_data;
          session_key.serialize(key_data);

          auto key_crypted = cert.public_key().encrypt(key_data.data());
                    
          dataflow(
            symmetric_encrypt(this->sp_, session_key),
            collect_data())(
              [this, done, from, session_key, key_crypted](const void * data, size_t size) {
                const_data_buffer crypted_data(data, size);

                auto server_log = this->storage_log_.get(this->sp_);
                
                binary_serializer to_sign;
                to_sign
                  << server_log.server_certificate().str()
                  << key_crypted
                  << crypted_data;

                auto message_data = udp_messages::welcome_message(
                  server_log.server_certificate().str(),
                  key_crypted,
                  crypted_data,
                  asymmetric_sign::signature(
                    hash::sha256(),
                    server_log.server_private_key(),
                    to_sign.data()))
                  .serialize();

                this->message_queue_.push(
                  network_service::get_ip_address_string(*from),
                  ntohs(from->sin_port),
                  message_data);

                done();
              },
              on_error,
              s.data().data(),
              s.data().size());
        }
        
        //server.get_peer_network().register_client_channel(
        //  server_certificate::server_id(cert),
        //  this);        
        
        break;
      }
      
      case udp_messages::message_identification::welcome_message_id:
      {
        this->log_.debug("welcome from %s", network_service::to_string(*from).c_str());

        udp_messages::welcome_message msg(s);

        auto cert = certificate::parse(msg.server_certificate());

        binary_serializer to_sign;
        to_sign
          << msg.server_certificate()
          << msg.key_crypted()
          << msg.crypted_data();

        if (asymmetric_sign_verify::verify(
          hash::sha256(),
          cert.public_key(),
          msg.sign(),
          to_sign.data())) {

          this->log_.debug(
            "welcome from %s has been accepted",
            network_service::to_string(*from).c_str());
          
          auto storage_log = this->sp_.get<istorage_log>();
          auto key_data = storage_log.server_private_key().decrypt(msg.key_crypted());

          symmetric_key session_key(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_data));
          dataflow(
            symmetric_decrypt(this->sp_, session_key),
            collect_data())(
              [this, done, &session_key, &cert, from](const void * data, size_t size) {
                uint32_t in_session_id;
                uint32_t out_session_id;

                binary_deserializer s(data, size);
                s >> out_session_id >> in_session_id;

                auto p = this->out_sessions_.find(out_session_id);
                if (this->out_sessions_.end() != p) {
                  p->second->init_session(
                    in_session_id,
                    network_service::get_ip_address_string(*from),
                    ntohs(from->sin_port),
                    std::move(cert),
                    std::move(session_key));

                  this->sp_.get<peer_network>().register_channel(
                    this,

                  );
                }
                
                done();
              },
              on_error,
              msg.crypted_data().data(),
              msg.crypted_data().size());
        }
        else {
          done();
        }

        break;
      }
      
      case udp_messages::message_identification::command_message_id:
      {
        this->log_.debug("command from %s", network_service::to_string(*from).c_str());

        guid client_id;
        uint64_t generation_id;
        const_data_buffer command_data;
        const_data_buffer sign_data;
        
        s >> client_id >> generation_id >> command_data >> sign_data;
        done();
                
        break;
      }
    }    
  });
}

void vds::_server_udp_api::update_upd_connection_pool()
{

}

void vds::_server_udp_api::send_welcome()
{
}

void vds::_server_udp_api::open_udp_session(const std::string & address)
{
  auto network_address = url_parser::parse_network_address(address);
  assert("udp" == network_address.protocol);
  
  auto server = network_address.server;
  auto port = (uint16_t)std::atoi(network_address.port.c_str());
  
  for(;;){
    auto session_id = (uint32_t)std::rand();
    if(this->out_sessions_.end() == this->out_sessions_.find(session_id)){
      
      this->out_sessions_[session_id].reset(new out_session_data(server, port));
  
      auto data = udp_messages::hello_message(
        this->storage_log_.get(this->sp_).server_certificate().str(),
        session_id,
        address).serialize();
      
      this->message_queue_.push(
        network_address.server,
        (uint16_t)std::atoi(network_address.port.c_str()),
        const_data_buffer(data));

      return;
    }
  }
}

vds::_server_udp_api::in_session_data::in_session_data(
  const guid & server_id,
  const symmetric_key & session_key)
  : server_id_(server_id), session_key_(session_key)
{
}

vds::_server_udp_api::out_session_data::out_session_data(
  const std::string & original_server,
  uint16_t original_port)
: original_server_(original_server),
  original_port_(original_port)
{
}

void vds::_server_udp_api::out_session_data::init_session(
  uint32_t external_session_id,
  const std::string & real_server,
  uint16_t real_port,
  certificate && cert,
  symmetric_key && session_key)
{
  this->external_session_id_ = external_session_id;
  this->real_server_ = real_server;
  this->real_port_ = real_port_;
  this->cert_.reset(new certificate(std::move(cert)));
  this->session_key_.reset(new symmetric_key(std::move(session_key)));
}
