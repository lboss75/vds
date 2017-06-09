#include "udp_socket.h"
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
#include "server_certificate.h"
#include "storage_log.h"
#include "server_log_sync_p.h"
#include "udp_socket_p.h"
#include "network_serializer.h"

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
  udp_channel_(new udp_channel(this))
{
}

vds::_connection_manager::~_connection_manager()
{
}

void vds::_connection_manager::start(const vds::service_provider& sp)
{
  std::map<std::string, std::string> endpoints;
  sp.get<node_manager>()->get_endpoints(sp, endpoints);
  
  for (auto & p : endpoints) {
    sp.get<logger>()->info(sp, "Connecting to %s", p.first.c_str());
    
    url_parser::parse_addresses(p.second,
      [this, sp](const std::string & protocol, const std::string & address) -> bool {
        if("udp" == protocol){
          this->udp_channel_->open_udp_session(sp, address);
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
    auto scope = sp.create_scope("Connect to " + address);
    imt_service::enable_async(scope);

    if ("udp" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_udp_channel(scope, na);
    }
    else if ("https" == protocol) {
      auto na = url_parser::parse_network_address(address);
      this->start_https_server(scope, na).wait(
        [this](const service_provider & sp) {
          sp.get<logger>()->info(sp, "HTTPS Servers stopped");
        },
        [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          sp.get<logger>()->info(sp, "HTTPS Server error: %s", ex->what());
        },
        scope);
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
  if (this->udp_channel_) {
    this->udp_channel_->broadcast(sp, message_type_id, get_binary, get_json);
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
: owner_(owner)
{
}

void vds::_connection_manager::udp_channel::start(
  const service_provider & sp,
  const url_parser::network_address& address)
{
  this->s_ = this->server_.start(sp, address.server, (uint16_t)std::atoi(address.port.c_str()));

  this->process_timer_.start(
    sp,
    std::chrono::seconds(5),
    [this, sp]() {
      return this->process_timer_jobs(sp);
    });
  
  this->schedule_read(sp);
}

void vds::_connection_manager::udp_channel::schedule_read(
  const service_provider & sp)
{
  this->s_.incoming()->read_async(sp, &this->input_message_, 1)
  .wait(
    [this](const service_provider & sp, size_t readed){
      if(0 < readed){
        this->input_message(
          sp,
          this->input_message_->addr(),
          this->input_message_.data(),
          this->input_message_.data_size())
        .wait(
          [this](const service_provider & sp) {
            this->schedule_read(sp);
          },
          [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<logger>()->error(sp, "Error at processing message");
            this->schedule_read(sp);
          },
          sp);

      }
    },
    [](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
      sp.unhandled_exception(ex);
    },
    sp);
}

void vds::_connection_manager::udp_channel::stop(const service_provider & sp)
{
  this->process_timer_.stop(sp);
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
  return create_async_task([this, from, data, len](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
    network_deserializer s(data, len);
    auto cmd = s.start();
    switch (cmd) {
    case udp_messages::message_identification::hello_message_id:
    {
      udp_messages::hello_message msg(s);
      sp.get<logger>()->debug(sp, "hello from %s", network_service::to_string(*from).c_str());

      //auto cert_manager = this->sp_.get<vds::cert_manager>();

      auto cert = certificate::parse_der(msg.source_certificate());
      //if(cert_manager.validate(cert))
      {
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

        auto data = std::make_shared<std::vector<uint8_t>>();
        dataflow(
          dataflow_arguments<uint8_t>(b.data().data(), b.data().size()),
          symmetric_encrypt(session.session_key()),
          collect_data(*data))(
            [this, done, on_error, from, &session, key_crypted, data](const service_provider & sp) {
          const_data_buffer crypted_data(data->data(), data->size());

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
          this->s_.outgoing()->write_all_async(sp, &data, 1)
            .wait(done, on_error, sp);
        },
          on_error,
          sp);
      }

      break;
    }

    case udp_messages::message_identification::welcome_message_id:
    {
      udp_messages::welcome_message msg(s);
      sp.get<logger>()->debug(sp, "welcome from %s", network_service::to_string(*from).c_str());

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

        sp.get<logger>()->debug(
          sp,
          "welcome from %s has been accepted",
          network_service::to_string(*from).c_str());

        auto storage_log = sp.get<istorage_log>();
        auto key_data = storage_log->server_private_key().decrypt(msg.key_crypted());

        symmetric_key session_key(symmetric_crypto::aes_256_cbc(), binary_deserializer(key_data));
        auto data = std::make_shared<std::vector<uint8_t>>();
        dataflow(
          dataflow_arguments<uint8_t>(msg.crypted_data().data(), msg.crypted_data().size()),
          symmetric_decrypt(session_key),
          collect_data(*data))(
            [this, done, &session_key, &cert, from, data](const service_provider & sp) {
          uint32_t in_session_id;
          uint32_t out_session_id;

          principal_log_record::record_id last_record_id;
          binary_deserializer s(data->data(), data->size());
          s >> out_session_id >> in_session_id >> last_record_id;

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

          sp.get<_server_log_sync>()->ensure_record_exists(sp, last_record_id);
          done(sp);
        },
            on_error,
          sp);
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

        sp.get<logger>()->debug(sp, "command from %s", network_service::to_string(*from).c_str());

        std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

        auto p = this->sessions_.find(session_id);
        if (this->sessions_.end() != p) {
          auto data = std::make_shared<std::vector<uint8_t>>();
          dataflow(
            dataflow_arguments<uint8_t>(crypted_data.data(), crypted_data.size()),
            symmetric_decrypt(p->second->session_key()),
            collect_data(*data)
          )(
            [this, &data_hash, session = p->second, data](const service_provider & sp) {
            if (hash::signature(hash::sha256(), data->data(), data->size()) == data_hash) {

              binary_deserializer d(data->data(), data->size());
              uint32_t message_type_id;
              const_data_buffer binary_form;
              d >> message_type_id >> binary_form;

              switch ((message_identification)message_type_id) {
              case message_identification::server_log_record_broadcast_message_id:
                sp.get<_server_log_sync>()->on_record_broadcast(
                  sp,
                  _server_log_sync::server_log_record_broadcast(sp, binary_form));
                break;

              case message_identification::server_log_get_records_broadcast_message_id:
                sp.get<_server_log_sync>()->on_server_log_get_records_broadcast(
                  sp,
                  *session.get(),
                  _server_log_sync::server_log_get_records_broadcast(binary_form));
                break;

              default:
                sp.get<logger>()->debug(sp, "Handler for message %d not found", message_type_id);
                break;
              }
            }
            else {
              sp.get<logger>()->debug(sp, "Invalid data hash");
            }
          },
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
          },
            sp);
        }
        else {
          sp.get<logger>()->debug(sp, "Session %d not found", session_id);
        }
      }
      catch (const std::exception & ex) {
        sp.get<logger>()->debug(sp, "Error at processing command");
        on_error(sp, std::make_shared<std::exception>(ex));
        return;
      }
      catch (...) {
        sp.get<logger>()->debug(sp, "Error at processing command");
        on_error(sp, std::make_shared<std::runtime_error>("Unhandled error"));
        return;
      }

      done(sp);
      break;
    }
    }
  });
}

void vds::_connection_manager::udp_channel::open_udp_session(
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
        sp.get<istorage_log>()->server_certificate().der(),
        session_id,
        address).serialize();
      
      auto scope = sp.create_scope("Send hello to " + server + ":" + std::to_string(port));
      imt_service::enable_async(scope);
      this->s_.outgoing()->write_value_async(scope, udp_datagram(server, port, data, false))
        .wait(
          [](const service_provider & sp) {},
          [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.unhandled_exception(ex);
          },
          scope);

      return;
    }
  }
}

void vds::_connection_manager::udp_channel::broadcast(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json)
{
  this->for_each_sessions([this, sp, message_type_id, &get_binary, &get_json](const session & session){
    session.send_to(sp, message_type_id, get_binary, get_json);
  });
}

vds::_connection_manager::udp_channel::session::session(
  udp_channel * owner,
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

vds::_connection_manager::udp_channel::session::~session()
{
}

void vds::_connection_manager::udp_channel::session::send_to(
  const service_provider & sp,
  uint32_t message_type_id,
  const std::function<const_data_buffer(void)> & get_binary,
  const std::function<std::string(void)> & get_json) const
{
  binary_serializer message_data;
  message_data << message_type_id << get_binary();

  auto crypted_data = std::make_shared<std::vector<uint8_t>>();
  dataflow(
    dataflow_arguments<uint8_t>(message_data.data().data(), message_data.data().size()),
    symmetric_encrypt(this->session_key()),
    collect_data(*crypted_data))(
      [this, sp, &message_data, crypted_data](const service_provider & sp) {
        network_serializer s;
        s.start(udp_messages::command_message_id);
        s << this->external_session_id();
        s.push_data(crypted_data->data(), crypted_data->size());
        s << hash::signature(hash::sha256(), message_data.data());
        s.final();

        auto scope = sp.create_scope("Send message to " + this->server() + ":" + std::to_string(this->port()));
        imt_service::enable_async(scope);
        this->owner_->s_.outgoing()->write_value_async(scope, udp_datagram(this->server(), this->port(), s.data()))
          .wait(
            [](const service_provider & sp) {},
            [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              sp.unhandled_exception(ex);
            },
            scope);
      },
      [](const service_provider & sp,
         const std::shared_ptr<std::exception> & ex) {
        std::rethrow_exception(std::make_exception_ptr(*ex)); },
      sp);
}

const vds::_connection_manager::udp_channel::incoming_session & 
vds::_connection_manager::udp_channel::register_incoming_session(
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

void vds::_connection_manager::udp_channel::for_each_sessions(
  const std::function<void(const session & session)> & callback)
{
  std::shared_lock<std::shared_mutex> lock(this->sessions_mutex_);

  for (auto & p : this->sessions_) {
    callback(*p.second);
  }
}

bool vds::_connection_manager::udp_channel::process_timer_jobs(const service_provider & sp)
{
  std::unique_lock<std::mutex> lock(this->hello_requests_mutex_);
  for (auto & p : this->hello_requests_) {
    sp.get<logger>()->debug(sp, "Reconect with %s:%d", p.second.server().c_str(), p.second.port());

    auto data = udp_messages::hello_message(
      sp.get<istorage_log>()->server_certificate().der(),
      p.first,
      "udp://" + p.second.server() + ":" + std::to_string(p.second.port())).serialize();

    auto scope = sp.create_scope("Send hello to " + p.second.server() + ":" + std::to_string(p.second.port()));
    imt_service::enable_async(scope);
    this->s_.outgoing()->write_value_async(scope, udp_datagram(p.second.server(), p.second.port(), data, false))
      .wait(
        [](const service_provider & sp) {},
        [](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          sp.unhandled_exception(ex); 
        },
        scope);
  }

  return true;
}

vds::_connection_manager::udp_channel::hello_request::hello_request()
{
}

vds::_connection_manager::udp_channel::hello_request::hello_request(
  uint32_t session_id,
  const std::string & server,
  uint16_t port)
: session_id_(session_id),
  server_(server),
  port_(port)
{
}

vds::_connection_manager::udp_channel::outgoing_session::outgoing_session(
  udp_channel * owner,
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

vds::_connection_manager::udp_channel::incoming_session::incoming_session(
  udp_channel * owner,
  uint32_t session_id,
  const std::string & server,
  uint16_t port,
  uint32_t external_session_id,
  const guid & server_id,
  const symmetric_key & session_key)
: session(owner, session_id, server, port, external_session_id, server_id, session_key)
{
}

