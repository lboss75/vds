/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "connection_manager.h"
#include "connection_manager_p.h"
#include "udp_messages.h"
#include "node_manager.h"

vds::connection_manager::connection_manager(const std::string & server_addresses)
: server_addresses_(server_addresses)
{
}

vds::connection_manager::~connection_manager()
{
}

void vds::connection_manager::register_services(vds::service_registrator & registrator)
{
  registrator.add_factory<iconnection_manager>([this](const service_provider &, bool &)->iconnection_manager {
    return iconnection_manager(this->impl_.get());
  });
}

void vds::connection_manager::start(const vds::service_provider& sp)
{
  this->impl_.reset(new _connection_manager(sp, this, this->server_addresses_));
  this->impl_->start();
}

void vds::connection_manager::stop(const vds::service_provider&)
{
  this->impl_->stop();
  this->impl_.reset();
}
//////////////////////////////////////////////////////
vds::iconnection_manager::iconnection_manager(vds::_connection_manager* owner)
: owner_(owner)
{
}

//////////////////////////////////////////////////////
vds::_connection_manager::_connection_manager(
  const vds::service_provider& sp,
  connection_manager * owner,
  const std::string & server_addresses)
: sp_(sp),
  owner_(owner),
  log_(sp, "Connection manager"),
  server_addresses_(server_addresses)
{
}

vds::_connection_manager::~_connection_manager()
{
}

void vds::_connection_manager::start()
{
  url_parser::parse_addresses(this->server_addresses_,
    [this](const std::string & protocol, const std::string & address) -> bool {
      if("udp" == protocol){
        auto na = url_parser::parse_network_address(address);
        this->start_udp_server(na).wait(
          [this](){this->log_.info("UPD Servers stopped");},
          [this](std::exception_ptr ex){this->log_.error("UPD Server error: %s", exception_what(ex));}
        );
      }
      else if ("https" == protocol) {
        auto na = url_parser::parse_network_address(address);
        this->start_https_server(na).wait(
          [this](){this->log_.info("HTTOS Servers stopped");},
          [this](std::exception_ptr ex){this->log_.error("HTTPS Server error: %s", exception_what(ex));}
        );
      }
    
      return true;
  });
  
  
  std::map<std::string, std::string> endpoints;
  this->sp_.get<node_manager>().get_endpoints(endpoints);
  
  for (auto & p : endpoints) {
    this->log_.info("Connecting to %s", p.first.c_str());
    
    url_parser::parse_addresses(p.second,
      [this](const std::string & protocol, const std::string & address) -> bool {
        if("udp" == protocol){
          this->udp_server_->open_udp_session(address);
        }
        else if ("https" == protocol) {
          //this->open_https_session(address);
        }
      
        return true;
    });
  }
}

vds::async_task<> vds::_connection_manager::start_server(const std::string& address)
{
  auto network = url_parser::parse_network_address(address);
  if("udp" == network.protocol){
    return this->start_udp_server(network);
  }
  else if ("https" == network.protocol) {
    return this->start_https_server(network);
  }
  else {
    throw std::runtime_error("Unexpected protocol at start server by address '" + address + "'");
  }
}


vds::async_task<> vds::_connection_manager::start_udp_server(const url_parser::network_address& address)
{
  this->udp_server_.reset(new udp_server(this));
  return this->udp_server_->start(address);
}

vds::async_task<> vds::_connection_manager::start_https_server(const url_parser::network_address& address)
{
  throw std::runtime_error("Not implemented");
}

//////////////////////////////////////////////////////
vds::_connection_manager::udp_server::udp_server(
  _connection_manager * owner)
: owner_(owner),
  sp_(owner->sp_),
  s_(owner->sp_),
  log_(owner->sp_, "UDP Server"),
  message_queue_(owner->sp_)
{
}

vds::async_task<> vds::_connection_manager::udp_server::start(
  const url_parser::network_address& address)
{
  return run_udp_server<vds::_connection_manager::udp_server>(
    this->owner_->sp_,
    this->s_,
    address.server,
    (uint16_t)std::atoi(address.port.c_str()),
    *this);
}

vds::async_task<> vds::_connection_manager::udp_server::input_message(
  const sockaddr_in * from,
  const void * data,
  size_t len)
{
  return create_async_task([this, from, data, len](const std::function<void(void)> & done, const error_handler & on_error){
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch(cmd) {
      case udp_messages::message_identification::hello_message_id:
      {
        this->log_.debug("hello from %s", network_service::to_string(*from).c_str());
        udp_messages::hello_message msg(s);
        
        //auto cert_manager = this->sp_.get<vds::cert_manager>();
        
        auto cert = certificate::parse(msg.source_certificate());
        //if(cert_manager.validate(cert))
        {
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

//                   this->sp_.get<iconnection_manager>()
//                     .register_channel(
//                     );
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

void vds::_connection_manager::udp_server::open_udp_session(const std::string & address)
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

vds::_connection_manager::udp_server::in_session_data::in_session_data(
  const guid & server_id,
  const symmetric_key & session_key)
  : server_id_(server_id), session_key_(session_key)
{
}

vds::_connection_manager::udp_server::out_session_data::out_session_data(
  const std::string & original_server,
  uint16_t original_port)
: original_server_(original_server),
  original_port_(original_port)
{
}

void vds::_connection_manager::udp_server::out_session_data::init_session(
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
