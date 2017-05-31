/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_connection.h"
#include "http_serializer.h"
#include "http_parser.h"

vds::client_connection::client_connection(const std::string & address, int port, certificate * client_certificate, asymmetric_private_key * client_private_key)
: address_(address),
  port_(port),
  client_certificate_(client_certificate),
  client_private_key_(client_private_key),
  state_(STATE::NONE),
  incoming_stream_(new async_stream<std::shared_ptr<http_message>>()),
  outgoing_stream_(new async_stream<std::shared_ptr<http_message>>())
{
}

vds::client_connection::~client_connection()
{
}



void vds::client_connection::connect(const service_provider & sp)
{
  this->state_ = STATE::CONNECTING;
  this->connection_start_ = std::chrono::steady_clock::now();

  auto scope = sp.create_scope("Client");
  imt_service::enable_async(scope);
  tcp_network_socket::connect(
    sp,
    this->address_,
    this->port_)
    .then(
      [this](
        const std::function<void(const service_provider & sp)> & done,
        const error_handler & on_error,
        const service_provider & sp,
        tcp_network_socket && s) {

    sp.get<logger>()->debug(sp, "Connected");
    auto client_crypto_tunnel = std::make_shared<ssl_tunnel>(true, this->client_certificate_, this->client_private_key_);

    async_series(
      create_async_task(
        [this, s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
      dataflow(
        read_tcp_network_socket(s, this->cancellation_source_.token()),
        stream_write<uint8_t>(client_crypto_tunnel->crypted_input())
      )([done](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Client crypted input closed");
        done(sp);
      },
        [on_error](const service_provider & sp, std::exception_ptr ex) {
        sp.get<logger>()->debug(sp, "Client crypted input error");
        on_error(sp, ex);
      },
        sp.create_scope("Client SSL Input"));
    }),
      create_async_task(
        [this, s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
      dataflow(
        stream_read<uint8_t>(client_crypto_tunnel->crypted_output()),
        write_tcp_network_socket(s, this->cancellation_source_.token())
      )([done](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Client crypted output closed");
        done(sp);
      },
        [on_error](const service_provider & sp, std::exception_ptr ex) {
        sp.get<logger>()->debug(sp, "Client crypted output error");
        on_error(sp, ex);
      }, sp.create_scope("Client SSL Output"));
    }),
      create_async_task(
        [this, s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
      dataflow(
        stream_read<std::shared_ptr<http_message>>(this->incoming_stream_),
        http_serializer(),
        stream_write<uint8_t>(client_crypto_tunnel->decrypted_input())
      )(
        [done](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Client writer closed");
        done(sp);
      },
        [on_error](const service_provider & sp, std::exception_ptr ex) {
        sp.get<logger>()->debug(sp, "Client writer error");
        on_error(sp, ex);
      },
        sp.create_scope("Client writer"));

    }),
      create_async_task(
        [this, s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
      dataflow(
        stream_read<uint8_t>(client_crypto_tunnel->decrypted_output()),
        http_parser(
          [this, s, done, on_error](const service_provider & sp, const std::shared_ptr<http_message> & request) {
            
            this->incoming_stream_->write_value_async(sp, request)
            .wait([](const service_provider & sp) {},
                  [](const service_provider & sp, std::exception_ptr ex) { sp.unhandled_exception(ex); },
                  sp);
        })
      )(
        [done](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Client reader closed");
        done(sp);
      },
        [on_error](const service_provider & sp, std::exception_ptr ex) {
        sp.get<logger>()->debug(sp, "Client reader error");
        on_error(sp, ex);
      },
        sp.create_scope("Client reader"));
    })
      ).wait(
        [client_crypto_tunnel](const service_provider & sp) {
      sp.get<logger>()->debug(sp, "Client closed");
    },
        [on_error](const service_provider & sp, std::exception_ptr ex) {
      sp.get<logger>()->debug(sp, "Client error");
      on_error(sp, ex);
    },
      sp.create_scope("Client dataflow"));
    client_crypto_tunnel->start(sp);

    done(sp);

  })
    .wait(
      [this](const service_provider & sp) {
        this->state_ = STATE::CONNECTED;
        this->connection_start_ = std::chrono::steady_clock::now();
      },
      [this](const service_provider & sp, std::exception_ptr ex) {
        this->state_ = STATE::CONNECT_ERROR;
        sp.unhandled_exception(ex);        
      },
      scope);
}
