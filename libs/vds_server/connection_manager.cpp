/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include <set>
#include "udp_socket.h"
#include "connection_manager.h"
#include "private/connection_manager_p.h"
#include "udp_messages.h"
#include "node_manager.h"
#include "messages.h"
#include "server_certificate.h"
#include "storage_log.h"
#include "private/server_log_sync_p.h"
#include "private/udp_socket_p.h"
#include "network_serializer.h"
#include "private/chunk_manager_p.h"
#include "private/route_manager_p.h"
#include "private/object_transfer_protocol_p.h"
#include "private/server_database_p.h"

vds::connection_manager::connection_manager()
  : impl_(new _connection_manager(this))
{
}

vds::connection_manager::~connection_manager()
{
}

void vds::connection_manager::register_services(vds::service_registrator & registrator)
{
  registrator.add_service<iconnection_manager>(this->impl_.get());
}

void vds::connection_manager::start(const vds::service_provider& sp)
{
  this->impl_->start(sp);
}

void vds::connection_manager::stop(const vds::service_provider& sp)
{
  this->impl_->stop(sp);
}

void vds::connection_manager::set_addresses(
  const std::string & server_addresses)
{
  this->impl_->set_addresses(server_addresses);
}

//////////////////////////////////////////////////////

void vds::iconnection_manager::send_transfer_request(
  const service_provider & sp,
  const guid & server_id,
  uint64_t index)
{
  static_cast<_connection_manager *>(this)->send_transfer_request(sp, server_id, index);
}

void vds::iconnection_manager::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  static_cast<_connection_manager *>(this)->broadcast(sp, message_type_id, message_data);
}

void vds::iconnection_manager::send_to(
  const service_provider & sp,
  const connection_session & session,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  static_cast<_connection_manager *>(this)->send_to(sp, session, message_type_id, message_data);
}

void vds::iconnection_manager::send_to(
  const service_provider & sp,
  const guid & server_id,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  static_cast<_connection_manager *>(this)->send_to(sp, server_id, message_type_id, message_data);
}
//////////////////////////////////////////////////////
vds::_connection_manager::_connection_manager(
  connection_manager * owner)
: owner_(owner),
  udp_channel_(new udp_channel(this))
{
}

vds::_connection_manager::~_connection_manager()
{
}

void vds::_connection_manager::start(const vds::service_provider& sp)
{
  url_parser::parse_addresses(this->server_addresses_,
    [this, sp](const std::string & protocol, const std::string & address) -> bool {
    auto scope = sp.create_scope(("Connect to " + address).c_str());
    imt_service::enable_async(scope);

    if ("udp" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_udp_channel(scope, na);
    }
    else if ("https" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_https_server(scope, na)
              .execute(
        [this, sp = scope](const std::shared_ptr<std::exception> & ex) {
            if(!ex){
        sp.get<logger>()->info("HTTPS", sp, "Servers stopped");
      } else {
        sp.get<logger>()->info("HTTPS", sp, "Server error: %s", ex->what());
      }});
    }

    return true;
  });

  barrier b;
  (*sp.get<iserver_database>())->get_db()->sync_transaction(sp,
    [this, sp, &b](database_transaction & t){

    std::map<std::string, std::string> endpoints;
    sp.get<node_manager>()->get_endpoints(sp, t, endpoints);

    async_task<> result = async_task<>::empty();
    
    for (auto & p : endpoints) {
      auto address = p.second;
      result = result.then([this, sp, address](){ return this->try_to_connect(sp, address);});
    }
    
    auto mt_scope = sp.create_scope("Staring connection manager");
    mt_service::enable_async(mt_scope);
    result.execute([sp, &b](const std::shared_ptr<std::exception> & ex){
      if(!ex){
      b.set();
    } else {
      sp.unhandled_exception(ex);
    }
    });
    
    return true;
  });
  
  b.wait();
}

vds::async_task<> vds::_connection_manager::try_to_connect(
  const vds::service_provider& sp,
  const std::string & address)
{
  sp.get<logger>()->info("network", sp, "Connecting to %s", address.c_str());
  
  async_task<> result = async_task<>::empty();

  url_parser::parse_addresses(address,
    [this, &result, sp](const std::string & protocol, const std::string & address) -> bool {
    if ("udp" == protocol) {
      result = result.then([this, sp, address](){
        return this->udp_channel_->open_udp_session(sp, address);
      });
    }
    else if ("https" == protocol) {
      //this->open_https_session(sp, address);
    }

    return true;
  });
  
  return result;
}

void vds::_connection_manager::stop(const vds::service_provider& sp)
{
}

void vds::_connection_manager::set_addresses(
  const std::string & server_addresses)
{
  this->server_addresses_ = server_addresses;
}

void vds::_connection_manager::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  if (this->udp_channel_) {
    this->udp_channel_->broadcast(sp, message_type_id, message_data);
  }
}

void vds::_connection_manager::send_transfer_request(
  const service_provider & sp,
  const guid & server_id,
  uint64_t index)
{
}

void vds::_connection_manager::enum_sessions(
  const std::function<bool (connection_session &)> & callback)
{
  this->udp_channel_->for_each_sessions(
    [callback](udp_channel::session & s)->bool {
      return callback(s);
    });
}

void vds::_connection_manager::possible_connections(
  const service_provider & sp,
  const std::list<trace_point> & trace_route)
{
  std::set<guid> exists;

  this->enum_sessions([&exists](connection_session & session)->bool {
      if (!session.address().empty() && exists.end() == exists.find(session.server_id())) {
        exists.emplace(session.server_id());
      }

      return true;
  });

  for(auto & p : trace_route) {
    if (exists.end() == exists.find(p.server_id())) {
      this->try_to_connect(sp, p.address());
    }
  }
}

void vds::_connection_manager::start_udp_channel(
  const service_provider & sp,
  const url_parser::network_address& address)
{
  return this->udp_channel_->start(sp, address);
}

vds::async_task<> vds::_connection_manager::start_https_server(
  const vds::service_provider& sp,
  const url_parser::network_address& /*address*/)
{
  throw std::runtime_error("Not implemented");
}

//////////////////////////////////////////////////////
vds::_connection_manager::udp_channel::udp_channel(
  _connection_manager * owner)
: owner_(owner), process_timer_("UDP channel")
{
}

void vds::_connection_manager::udp_channel::start(
  const service_provider & sp,
  const url_parser::network_address& address)
{
  //TODO: this->s_ = this->server_.start(sp, address.server, (uint16_t)std::atoi(address.port.c_str()));
  //TODO:!!!
  this->s_.read_async().execute([sp, this](const std::shared_ptr<std::exception> & /*ex*/, const udp_datagram & message) {
    this->input_message(sp, message->addr(), message.data(), message.data_size());
  });

  this->process_timer_.start(
    sp,
    std::chrono::seconds(5),
    [this, sp]() {
      return this->process_timer_jobs(sp);
    });
  
  sp.get<logger>()->debug("UDPAPI", sp, "Pipeline opened");
  //this->schedule_read(sp);
}

/*
void vds::_connection_manager::udp_channel::schedule_read(
  const service_provider & sp)
{
  this->s_.incoming()->read_async(sp, &this->input_message_, 1)
  .execute(
    [this, sp](const std::shared_ptr<std::exception> & ex, size_t readed){
      if(!ex){
      if(0 < readed){
        this->input_message(
          sp,
          this->input_message_->addr(),
          this->input_message_.data(),
          this->input_message_.data_size())
        .execute(
          [this, sp](const std::shared_ptr<std::exception> & ex) {
            if(!ex){
            this->schedule_read(sp);
          } else {
            sp.get<logger>()->error("UDPAPI", sp, "Error %s at processing message", ex->what());
            this->schedule_read(sp);
          }});
      }
      else {
        sp.get<logger>()->debug("UDPAPI", sp, "Pipeline closed");
      }
    } else {
      sp.unhandled_exception(ex);
    }
    });
}
*/


void vds::_connection_manager::udp_channel::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);
  this->server_.stop(sp);
}

//void vds::_connection_manager::udp_channel::socket_closed(
//  const service_provider & sp,
//  const std::list<std::shared_ptr<std::exception>> & errors)
//{
//}


vds::async_task<> vds::_connection_manager::udp_channel::input_message(
  const vds::service_provider& sp,
  const sockaddr_in * from,
  const void * data,
  size_t len)
{
  sp.get<logger>()->trace("UDPAPI", sp, "message(%d) from %s", len, network_service::to_string(*from).c_str());
  
  return [this, sp, from, data, len](const async_result<> & result) {
    (*sp.get<iserver_database>())->get_db()->async_transaction(sp,
      [this, sp, from, data, len, result](database_transaction & tr)->bool{
    
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch (cmd) {
    case udp_messages::message_identification::hello_message_id:
    {
      udp_messages::hello_message msg(s);
      sp.get<logger>()->debug("UDPAPI", sp, "hello from %s", network_service::to_string(*from).c_str());

      //auto cert_manager = this->sp_.get<vds::cert_manager>();

      auto cert = certificate::parse_der(msg.source_certificate());
      //if(cert_manager.validate(cert))
      if(sp.get<istorage_log>()->current_server_id() != server_certificate::server_id(cert)) {
        auto session = this->register_incoming_session(
          network_service::get_ip_address_string(*from),
          ntohs(from->sin_port),
          msg.session_id(),
          server_certificate::server_id(cert));

        auto last_record_id = sp.get<istorage_log>()->get_last_applied_record(sp);
        binary_serializer b;
        b
          << msg.session_id()
          << session.session_id()
          << last_record_id;

        binary_serializer key_data;
        session.session_key().serialize(key_data);

        auto key_crypted = cert.public_key().encrypt(key_data.data());

        auto crypted_data = symmetric_encrypt::encrypt(session.session_key(), b.data().data(), b.data().size());

          auto server_log = sp.get<istorage_log>();

          binary_serializer to_sign;
          to_sign
            << server_log->server_certificate().str()
            << key_crypted
            << crypted_data;

          auto message_data = udp_messages::welcome_message(
            server_log->server_certificate().str(),
            key_crypted,
            crypted_data,
            asymmetric_sign::signature(
              hash::sha256(),
              server_log->server_private_key(),
              to_sign.data()))
            .serialize();

          auto data = _udp_datagram::create(*from, message_data);
/*TODO:
          auto stream = this->s_.outgoing();
          stream->write_value_async(sp, data)
          .execute(
            [stream, sp, result](const std::shared_ptr<std::exception> & ex){
              if(!ex){
              result.done();
            } else {
              result.error(ex);
            }
            });
*/
      }
      else {
        result.done();
      }

      break;
    }

    case udp_messages::message_identification::welcome_message_id:
    {
      udp_messages::welcome_message msg(s);
      sp.get<logger>()->debug("UDPAPI", sp, "welcome from %s", network_service::to_string(*from).c_str());

      auto cert = certificate::parse(msg.server_certificate());

      binary_serializer to_sign;
      to_sign
        << msg.server_certificate()
        << msg.key_crypted()
        << msg.crypted_data();

      if (asymmetric_sign_verify::verify(hash::sha256(), cert.public_key(), msg.sign(), to_sign.data())) {

        sp.get<logger>()->debug(
          "UDPAPI",
          sp,
          "welcome from %s has been accepted",
          network_service::to_string(*from).c_str());

        auto storage_log = sp.get<istorage_log>();
        auto key_data = storage_log->server_private_key().decrypt(msg.key_crypted());

        auto session_key = symmetric_key::deserialize(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_data));
        auto data = symmetric_decrypt::decrypt(session_key, msg.crypted_data().data(), msg.crypted_data().size());
        
              uint32_t in_session_id;
              uint32_t out_session_id;

              principal_log_record::record_id last_record_id;
              binary_deserializer s(data.data(), data.size());
              s >> out_session_id >> in_session_id >> last_record_id;

              std::lock_guard<std::mutex> lock_hello(this->hello_requests_mutex_);
              auto p = this->hello_requests_.find(out_session_id);
              if (this->hello_requests_.end() != p) {
                this->owner_->route_manager_->on_session_started(
                  sp,
                  sp.get<istorage_log>()->current_server_id(),
                  server_certificate::server_id(cert),
                  p->second.address());
                std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);

                auto network_address = url_parser::parse_network_address(p->second.address());
                assert("udp" == network_address.protocol);

                this->sessions_[out_session_id].reset(new outgoing_session(
                  this,
                  p->second.session_id(),
                  p->second.address(),
                  network_address.server,
                  (uint16_t)std::atoi(network_address.port.c_str()),
                  in_session_id,
                  network_service::get_ip_address_string(*from),
                  ntohs(from->sin_port),
                  std::move(cert),
                  std::move(session_key)));

                this->hello_requests_.erase(p);
              }

              sp.get<_server_log_sync>()->ensure_record_exists(sp, tr, last_record_id);
              result.done();
      }

      break;
    }

    case udp_messages::message_identification::command_message_id:
    {
      try {

        uint32_t session_id;
        const_data_buffer crypted_data;
        const_data_buffer data_hash;
        s >> session_id >> crypted_data >> data_hash;
        s.final();

        sp.get<logger>()->trace("UDPAPI", sp, "command from %s", network_service::to_string(*from).c_str());

        std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

        auto p = this->sessions_.find(session_id);
        if (this->sessions_.end() != p) {
          auto data = symmetric_decrypt::decrypt(p->second->session_key(), crypted_data.data(), crypted_data.size());
          
            if (hash::signature(hash::sha256(), data.data(), data.size()) == data_hash) {

              binary_deserializer d(data.data(), data.size());
              uint32_t message_type_id;
              const_data_buffer binary_form;
              d >> message_type_id >> binary_form;
              
              sp.get<logger>()->trace("UDPAPI", sp, "Message %d", message_type_id);

              this->owner_->server_to_server_api_.process_message(
                sp,
                tr,
                this->owner_,
                *p->second.get(),
                message_type_id,
                binary_form);
            }
            else {
              sp.get<logger>()->error("UDPAPI", sp, "Invalid data hash");
            }
        }
        else {
          sp.get<logger>()->warning("UDPAPI", sp, "Session %d not found", session_id);
        }
      }
      catch (const std::exception & ex) {
        sp.get<logger>()->error("UDPAPI", sp, "Error at processing command");
        result.error(std::make_shared<std::exception>(ex));
        return false;
      }
      catch (...) {
        sp.get<logger>()->error("UDPAPI", sp, "Error at processing command");
        result.error(std::make_shared<std::runtime_error>("Unhandled error"));
        return false;
      }

      result.done();
      break;
    }
    }

    return true;
      });
  };
}

void vds::_connection_manager::send_to(
  const service_provider & sp,
  const guid & server_id,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  this->route_manager_.send_to(
    sp,
    server_id,
    message_type_id,
    message_data);
}

vds::async_task<> vds::_connection_manager::udp_channel::open_udp_session(
  const service_provider & sp,
  const std::string & address)
{
  return [this, sp, address](const async_result<> & result){
      auto network_address = url_parser::parse_network_address(address);
      assert("udp" == network_address.protocol);
      
      auto server = network_address.server;
      auto port = (uint16_t)std::atoi(network_address.port.c_str());
      
      std::lock_guard<std::mutex> lock_hello(this->hello_requests_mutex_);
      std::unique_lock<std::shared_mutex> lock_sessions(this->sessions_mutex_);
      
      for(;;){
        auto session_id = (uint32_t)std::rand();
        if(this->hello_requests_.end() == this->hello_requests_.find(session_id)
          && this->sessions_.end() == this->sessions_.find(session_id)
          ){
          this->hello_requests_[session_id] = hello_request(session_id, address);
      
          auto data = udp_messages::hello_message(
            sp.get<istorage_log>()->server_certificate().der(),
            session_id,
            address).serialize();
          
          auto scope = sp.create_scope(("Send hello to " + address).c_str());
          imt_service::enable_async(scope);
          
/*TODO:
          auto stream = this->s_.outgoing();
          stream->write_value_async(scope, udp_datagram(server, port, data, false))
          .execute([stream, result](const std::shared_ptr<std::exception> & ex) {
            if(!ex){
            result.done();
          } else {
            result.error(ex);
          }});
*/

          return;
        }
      }
  };
}

void vds::_connection_manager::udp_channel::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const const_data_buffer & message_data)
{
  this->for_each_sessions([this, sp, message_type_id, message_data](session & session)->bool{
    session.send_to(sp, message_type_id, message_data);
    return true;
  });
}

vds::_connection_manager::udp_channel::session::session(
  udp_channel * owner,
  uint32_t session_id,
  const std::string & address,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & partner_id,
  const symmetric_key & session_key)
: owner_(owner),
  session_id_(session_id),
  address_(address),
  server_(server),
  port_(port),
  external_session_id_(external_session_id),
  partner_id_(partner_id),
  session_key_(session_key)
{
}

vds::_connection_manager::udp_channel::session::~session()
{
}

void vds::_connection_manager::udp_channel::session::send_to(
  const service_provider & sp,
  uint32_t message_type_id,
  const const_data_buffer & msg_data) const
{
  binary_serializer message_data;
  message_data << message_type_id << msg_data;

  auto crypted_data = symmetric_encrypt::encrypt(this->session_key(), message_data.data().data(), message_data.data().size());
  
        network_serializer s;
        s.start(udp_messages::command_message_id);
        s << this->external_session_id();
        s.push_data(crypted_data.data(), crypted_data.size());
        s << hash::signature(hash::sha256(), message_data.data());
        s.final();

        sp.get<logger>()->trace("UDPAPI", sp, "Send message to %s:%d", this->server().c_str(), this->port());
        
        auto scope = sp.create_scope(("Send message to " + this->server() + ":" + std::to_string(this->port())).c_str());
        imt_service::enable_async(scope);
/*
        auto stream = this->owner_->s_.outgoing();
        stream->write_value_async(scope, udp_datagram(this->server(), this->port(), s.data()))
        .execute([stream](const std::shared_ptr<std::exception> & ex){});
*/
}

const vds::_connection_manager::udp_channel::incoming_session & 
vds::_connection_manager::udp_channel::register_incoming_session(
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & server_id)
{
  auto session_key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);

  uint32_t session_id;
  for (;;) {
    session_id = (uint32_t)std::rand();
    if (this->sessions_.end() == this->sessions_.find(session_id)
      && this->hello_requests_.end() == this->hello_requests_.find(session_id)) {
      break;
    }
  }

  auto result = new incoming_session(
    this,
    session_id,
    server,
    port,
    external_session_id,
    server_id,
    session_key);

  this->sessions_[session_id].reset(result);

  return *result;
}

void vds::_connection_manager::udp_channel::for_each_sessions(
  const std::function<bool (session & session)> & callback)
{
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

  for (auto & p : this->sessions_) {
    if(!callback(*p.second)){
      break;
    }
  }
}

bool vds::_connection_manager::udp_channel::process_timer_jobs(const service_provider & sp)
{
  std::unique_lock<std::mutex> lock(this->hello_requests_mutex_);
  for (auto & p : this->hello_requests_) {
    sp.get<logger>()->debug("UDPAPI", sp, "Reconect with %s", p.second.address().c_str());

    auto data = udp_messages::hello_message(
      sp.get<istorage_log>()->server_certificate().der(),
      p.first,
      p.second.address()).serialize();

    auto scope = sp.create_scope(("Send hello to " + p.second.address()).c_str());
    imt_service::enable_async(scope);

    auto network_address = url_parser::parse_network_address(p.second.address());
    assert("udp" == network_address.protocol);

    auto server = network_address.server;
    auto port = (uint16_t)std::atoi(network_address.port.c_str());

/*
    auto stream = this->s_.outgoing();
    stream->write_value_async(scope, udp_datagram(server, port, data, false))
    .execute([stream](const std::shared_ptr<std::exception> & ex){});
*/
  }

  return true;
}

vds::_connection_manager::udp_channel::hello_request::hello_request()
{
}

vds::_connection_manager::udp_channel::hello_request::hello_request(
  uint32_t session_id,
  const std::string & address)
  : session_id_(session_id),
  address_(address)
{
}

vds::_connection_manager::udp_channel::outgoing_session::outgoing_session(
  udp_channel * owner,
  uint32_t session_id,
  const std::string & address,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const std::string & real_server,
  uint16_t real_port,
  certificate && cert,
  const symmetric_key & session_key)
: session(
  owner,
  session_id,
  address,
  server,
  port,
  external_session_id,
  server_certificate::server_id(cert),
  session_key),
  real_server_(real_server),
  real_port_(real_port),
  cert_(std::move(cert))
{
}

vds::_connection_manager::udp_channel::incoming_session::incoming_session(
  udp_channel * owner,
  uint32_t session_id,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & server_id,
  const symmetric_key & session_key)
: session(owner, session_id, std::string(), server, port, external_session_id, server_id, session_key)
{
}

