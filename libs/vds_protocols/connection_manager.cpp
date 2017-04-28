/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "connection_manager.h"
#include "connection_manager_p.h"
#include "udp_messages.h"
#include "node_manager.h"
#include "messages.h"

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

void vds::connection_manager::start_servers(
  const service_provider & sp,
  const std::string & server_addresses)
{
  this->impl_->start_servers(sp, server_addresses);
}

//////////////////////////////////////////////////////

vds::async_task<> vds::iconnection_manager::download_object(
  const service_provider & sp,
  const guid & server_id,
  uint64_t index,
  const filename & target_file)
{
  return static_cast<_connection_manager *>(this)->download_object(sp, server_id, index, target_file);
}

void vds::iconnection_manager::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json)
{
  static_cast<_connection_manager *>(this)->broadcast(sp, message_type_id, get_binary, get_json);
}

void vds::iconnection_manager::send_to(
  const service_provider & sp,
  const connection_session & session,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json)
{
  static_cast<_connection_manager *>(this)->send_to(sp, session, message_type_id, get_binary, get_json);
}

//////////////////////////////////////////////////////
vds::_connection_manager::_connection_manager(
  connection_manager * owner)
: owner_(owner),
  udp_server_(new udp_server(this))
{
}

vds::_connection_manager::~_connection_manager()
{
}

void vds::_connection_manager::start(const vds::service_provider& sp)
{
  std::map<std::string, std::string> endpoints;
  sp.get<node_manager>().get_endpoints(sp, endpoints);
  
  for (auto & p : endpoints) {
    sp.get<logger>().info(sp, "Connecting to %s", p.first.c_str());
    
    url_parser::parse_addresses(p.second,
      [this, sp](const std::string & protocol, const std::string & address) -> bool {
        if("udp" == protocol){
          this->udp_server_->open_udp_session(sp, address);
        }
        else if ("https" == protocol) {
          //this->open_https_session(sp, address);
        }
      
        return true;
    });
  }
}

void vds::_connection_manager::stop(const vds::service_provider& sp)
{
}

void vds::_connection_manager::start_servers(
  const vds::service_provider& sp,
  const std::string & server_addresses)
{
  url_parser::parse_addresses(server_addresses,
    [this, sp](const std::string & protocol, const std::string & address) -> bool {
    if ("udp" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_udp_server(sp, na).wait(
        [this, sp]() {sp.get<logger>().info(sp, "UPD Servers stopped"); },
        [this, sp](std::exception_ptr ex) {sp.get<logger>().info(sp, "UPD Server error: %s", exception_what(ex)); }
      );
    }
    else if ("https" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_https_server(sp, na).wait(
        [this, sp]() {sp.get<logger>().info(sp, "HTTPS Servers stopped"); },
        [this, sp](std::exception_ptr ex) { sp.get<logger>().info(sp, "HTTPS Server error: %s", exception_what(ex)); }
      );
    }

    return true;
  });

}

void vds::_connection_manager::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json)
{
  if (this->udp_server_) {
    this->udp_server_->broadcast(sp, message_type_id, get_binary, get_json);
  }
}

vds::async_task<> vds::_connection_manager::download_object(
  const service_provider & sp,
  const guid & server_id,
  uint64_t index,
  const filename & target_file)
{
  throw std::runtime_error("Not implemented");
}

vds::async_task<> vds::_connection_manager::start_udp_server(
  const service_provider & sp,
  const url_parser::network_address& address)
{
  return this->udp_server_->start(sp, address);
}

vds::async_task<> vds::_connection_manager::start_https_server(
  const vds::service_provider& sp,
  const url_parser::network_address& /*address*/)
{
  throw std::runtime_error("Not implemented");
}

//////////////////////////////////////////////////////
vds::_connection_manager::udp_server::udp_server(
  _connection_manager * owner)
: owner_(owner)
{
}

vds::async_task<> vds::_connection_manager::udp_server::start(
  const service_provider & sp,
  const url_parser::network_address& address)
{
  this->process_timer_.start(
    sp,
    std::chrono::seconds(5),
    [this, sp]() {
      this->process_timer_jobs(sp);
    });

  return run_udp_server<vds::_connection_manager::udp_server>(
    sp,
    this->s_,
    address.server,
    (uint16_t)std::atoi(address.port.c_str()),
    *this);
}

void vds::_connection_manager::udp_server::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);
}

void vds::_connection_manager::udp_server::socket_closed(
  const service_provider & sp,
  std::list<std::exception_ptr> errors)
{
}


vds::async_task<> vds::_connection_manager::udp_server::input_message(
  const vds::service_provider& sp,
  const sockaddr_in * from,
  const void * data,
  size_t len)
{
  return create_async_task([this, sp, from, data, len](const std::function<void(void)> & done, const error_handler & on_error) {
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch (cmd) {
    case udp_messages::message_identification::hello_message_id:
    {
      udp_messages::hello_message msg(s);
      sp.get<logger>().debug(sp, "hello from %s", network_service::to_string(*from).c_str());

      //auto cert_manager = this->sp_.get<vds::cert_manager>();

      auto cert = certificate::parse(msg.source_certificate());
      //if(cert_manager.validate(cert))
      {
        auto session = this->register_incoming_session(
          network_service::get_ip_address_string(*from),
          ntohs(from->sin_port),
          msg.session_id(),
          server_certificate::server_id(cert));

        binary_serializer b;
        b
          << msg.session_id()
          << session.session_id();

        binary_serializer key_data;
        session.session_key().serialize(key_data);

        auto key_crypted = cert.public_key().encrypt(key_data.data());

        dataflow(
          symmetric_encrypt(session.session_key()),
          collect_data())(
            [this, sp, done, from, &session, key_crypted](const void * data, size_t size) {
          const_data_buffer crypted_data(data, size);

          auto server_log = sp.get<istorage_log>();

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
            sp,
            network_service::get_ip_address_string(*from),
            ntohs(from->sin_port),
            message_data);
          done();
        },
            on_error,
          b.data().data(),
          b.data().size());
      }

      break;
    }

    case udp_messages::message_identification::welcome_message_id:
    {
      udp_messages::welcome_message msg(s);
      sp.get<logger>().debug(sp, "welcome from %s", network_service::to_string(*from).c_str());

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

        sp.get<logger>().debug(
          sp,
          "welcome from %s has been accepted",
          network_service::to_string(*from).c_str());

        auto storage_log = sp.get<istorage_log>();
        auto key_data = storage_log.server_private_key().decrypt(msg.key_crypted());

        symmetric_key session_key(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_data));
        dataflow(
          symmetric_decrypt(session_key),
          collect_data())(
            [this, done, &session_key, &cert, from](const void * data, size_t size) {
          uint32_t in_session_id;
          uint32_t out_session_id;

          binary_deserializer s(data, size);
          s >> out_session_id >> in_session_id;

          std::lock_guard<std::mutex> lock_hello(this->hello_requests_mutex_);
          auto p = this->hello_requests_.find(out_session_id);
          if (this->hello_requests_.end() != p) {
            std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);

            this->sessions_[out_session_id].reset(new outgoing_session(
              this,
              p->second,
              in_session_id,
              network_service::get_ip_address_string(*from),
              ntohs(from->sin_port),
              std::move(cert),
              std::move(session_key)));

            this->hello_requests_.erase(p);
          }

          done();
        },
            on_error,
          sp,
          msg.crypted_data().data(),
          msg.crypted_data().size());
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

        sp.get<logger>().debug(sp, "command from %s", network_service::to_string(*from).c_str());

        std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

        auto p = this->sessions_.find(session_id);
        if (this->sessions_.end() != p) {
          dataflow(
            symmetric_decrypt(p->second->session_key()),
            collect_data()
          )(
            [this, sp, &data_hash, session = p->second](const void * data, size_t size) {
            if (hash::signature(hash::sha256(), data, size) == data_hash) {

              binary_deserializer d(data, size);
              uint32_t message_type_id;
              const_data_buffer binary_form;
              d >> message_type_id >> binary_form;

              //TODO: auto h = this->owner_->input_message_handlers_.find(message_type_id);
              //if (this->owner_->input_message_handlers_.end() != h) {
                //h->second(*session, binary_form);
              //}
              //else {
                sp.get<logger>().debug(sp, "Handler for message %d not found", message_type_id);
              //}
            }
            else {
              sp.get<logger>().debug(sp, "Invalid data hash");
            }
          },
            [](std::exception_ptr ex) {
          },
            crypted_data.data(),
            crypted_data.size(),
            sp);
        }
        else {
          sp.get<logger>().debug(sp, "Session %d not found", session_id);
        }
      }
      catch (...) {
        sp.get<logger>().debug(sp, "Error at processing command");
        on_error(std::current_exception());
        return;
      }

      done();
      break;
    }
    }
  });
}

void vds::_connection_manager::udp_server::open_udp_session(
  const service_provider & sp,
  const std::string & address)
{
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
      this->hello_requests_[session_id] = hello_request(session_id, server, port);
  
      auto data = udp_messages::hello_message(
        sp.get<istorage_log>().server_certificate().str(),
        session_id,
        address).serialize();
      
      this->message_queue_.push(
        sp,
        server,
        port,
        const_data_buffer(data));

      return;
    }
  }
}

void vds::_connection_manager::udp_server::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json)
{
  this->for_each_sessions([this, sp, message_type_id, &get_binary, &get_json](const session & session){
    session.send_to(sp, message_type_id, get_binary, get_json);
  });
}

vds::_connection_manager::udp_server::session::session(
  udp_server * owner,
  uint32_t session_id,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & partner_id,
  const symmetric_key & session_key)
: owner_(owner),
  session_id_(session_id),
  server_(server),
  port_(port),
  external_session_id_(external_session_id),
  partner_id_(partner_id),
  session_key_(session_key)
{
}

vds::_connection_manager::udp_server::session::~session()
{
}

void vds::_connection_manager::udp_server::session::send_to(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json) const
{
  binary_serializer message_data;
  message_data << message_type_id << get_binary();

  dataflow(
    symmetric_encrypt(this->session_key()),
    collect_data())(
      [this, sp, &message_data](const void * crypted_data, size_t size) {
        network_serializer s;
        s.start(udp_messages::command_message_id);
        s << this->external_session_id();
        s.push_data(crypted_data, size);
        s << hash::signature(hash::sha256(), message_data.data());
        s.final();

        this->owner_->message_queue_.push(
          sp,
          this->server(),
          this->port(),
          s.data());
      },
      [](std::exception_ptr ex) { std::rethrow_exception(ex); },
      sp,
      message_data.data().data(),
      message_data.data().size());
}

const vds::_connection_manager::udp_server::incoming_session & 
vds::_connection_manager::udp_server::register_incoming_session(
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & server_id)
{
  symmetric_key session_key(symmetric_crypto::aes_256_cbc());
  session_key.generate();

  std::unique_lock<std::shared_mutex> lock(this->sessions_mutex_);

  uint32_t session_id;
  for (;;) {
    session_id = (uint32_t)std::rand();
    if (this->sessions_.end() == this->sessions_.find(session_id)) {
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

void vds::_connection_manager::udp_server::for_each_sessions(
  const std::function<void(const session & session)> & callback)
{
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

  for (auto & p : this->sessions_) {
    callback(*p.second);
  }
}

void vds::_connection_manager::udp_server::process_timer_jobs(const service_provider & sp)
{
  std::unique_lock<std::mutex> lock(this->hello_requests_mutex_);
  for (auto & p : this->hello_requests_) {
    sp.get<logger>().debug(sp, "Reconect with %s:%d", p.second.server().c_str(), p.second.port());

    auto data = udp_messages::hello_message(
      sp.get<istorage_log>().server_certificate().str(),
      p.first,
      "udp://" + p.second.server() + ":" + std::to_string(p.second.port())).serialize();

    this->message_queue_.push(
      sp,
      p.second.server(),
      p.second.port(),
      const_data_buffer(data));
  }
}

vds::_connection_manager::udp_server::hello_request::hello_request()
{
}

vds::_connection_manager::udp_server::hello_request::hello_request(
  uint32_t session_id,
  const std::string & server,
  uint16_t port)
: session_id_(session_id),
  server_(server),
  port_(port)
{
}

vds::_connection_manager::udp_server::outgoing_session::outgoing_session(
  udp_server * owner,
  const hello_request & original_request,
  uint32_t external_session_id,
  const std::string & real_server,
  uint16_t real_port,
  certificate && cert,
  const symmetric_key & session_key)
: session(
  owner,
  original_request.session_id(),
  original_request.server(),
  original_request.port(),
  external_session_id,
  server_certificate::server_id(cert),
  session_key),
  real_server_(real_server),
  real_port_(real_port),
  cert_(std::move(cert))
{
}

vds::_connection_manager::udp_server::incoming_session::incoming_session(
  udp_server * owner,
  uint32_t session_id,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & server_id,
  const symmetric_key & session_key)
: session(owner, session_id, server, port, external_session_id, server_id, session_key)
{
}

