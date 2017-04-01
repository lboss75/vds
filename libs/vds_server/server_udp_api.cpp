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
  const service_provider& sp,
  certificate & certificate,
  asymmetric_private_key & private_key)
: impl_(new _server_udp_api(sp, this, certificate, private_key))
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
  server_udp_api * owner,
  certificate & certificate,
  asymmetric_private_key & private_key)
: sp_(sp),
  log_(sp, "Server UDP API"),
  owner_(owner),
  certificate_(certificate),
  private_key_(private_key),
  s_(sp),
  out_session_id_(0),
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

void vds::_server_udp_api::input_message(const sockaddr_in * from, const void * data, size_t len)
{
  try{
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch(cmd) {
      case udp_messages::message_identification::hello_message_id:
      {
        udp_messages::hello_message msg(s);
        
        auto server = this->sp_.get<iserver>();
        auto cert_manager = server.get_cert_manager();
        
        auto cert = certificate::parse(msg.source_certificate());
        if(cert_manager.validate(cert)){

          symmetric_key session_key(symmetric_crypto::aes_256_cbc());
          session_key.generate();

          binary_serializer key_data;
          session_key.serialize(key_data);

          auto key_crypted = this->certificate_.public_key().encrypt(key_data.data());

          auto session_id = ++this->in_last_session_;
          
          auto server_id = server_certificate::server_id(cert);
          auto session = new session_data(server_id, session_key);
          
          binary_serializer s;
          s << session_id;

          barrier b;
          data_buffer crypted_data;
          dataflow(
            symmetric_encrypt(this->sp_, session_key),
            collect_data())(
              [&crypted_data, &b](const void * data, size_t size) { crypted_data.reset(data, size); b.set(); },
              [](std::exception_ptr ex) { std::rethrow_exception(ex); },
              s.data().data(),
              s.data().size());
          b.wait();

          binary_serializer to_sign;
          to_sign
            << server_certificate::server_id(this->certificate_)
            << key_crypted
            << crypted_data;

          network_serializer message_data;
          udp_messages::welcome_message(
            server_certificate::server_id(this->certificate_),
            key_crypted,
            crypted_data,
            asymmetric_sign::signature(
              hash::sha256(),
              this->private_key_,
              to_sign.data()))
            .serialize(message_data);

          this->message_queue_.push(
            network_service::get_ip_address_string(*from),
            from->sin_port,
            message_data.data());
        }
        
        //server.get_peer_network().register_client_channel(
        //  server_certificate::server_id(cert),
        //  this);        
        
        break;
      }
      
      case udp_messages::message_identification::welcome_message_id:
      {
        guid client_id;
        uint64_t generation_id;
        data_buffer mac_key;
        
        s >> client_id >> generation_id >> mac_key;
        
        //this->sp_.get<peer_network>().register_client_channel(
        //  client_id,
        //  this);
        break;
      }
      
      case udp_messages::message_identification::command_message_id:
      {
        guid client_id;
        uint64_t generation_id;
        data_buffer command_data;
        data_buffer sign_data;
        
        s >> client_id >> generation_id >> command_data >> sign_data;
                
        break;
      }
    }    
  }
  catch(std::exception * ex){
    std::unique_ptr<std::exception> _ex(ex);
    this->log_.warning("%s in datagram from %s", ex->what(), network_service::to_string(*from).c_str());
  }
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
  
  network_serializer s;
  
  udp_messages::hello_message(
    this->certificate_.str(),
    ++(this->out_session_id_),
    address).serialize(s);
  
  this->message_queue_.push(
    network_address.server,
    (uint16_t)std::atoi(network_address.port.c_str()),
    data_buffer(s.data()));
}


vds::_server_udp_api::session_data::session_data(const guid & server_id, const symmetric_key & session_key)
  : server_id_(server_id), session_key_(session_key)
{
}
