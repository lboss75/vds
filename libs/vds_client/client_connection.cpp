/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "client_connection.h"
#include "http_serializer.h"
#include "http_parser.h"

vds::client_connection::client_connection(
  const std::string & address,
  int port, 
  certificate * client_certificate,
  asymmetric_private_key * client_private_key)
: address_(address),
  port_(port),
  client_certificate_(client_certificate),
  client_private_key_(client_private_key),
  state_(STATE::NONE),
  incoming_stream_(new async_buffer<std::shared_ptr<http_message>>(*(service_provider *)nullptr)),
  outgoing_stream_(new continuous_buffer<std::shared_ptr<json_value>>(*(service_provider *)nullptr))
{
}

vds::client_connection::~client_connection()
{
}

void vds::client_connection::connect(const service_provider & sp)
{
//   this->state_ = STATE::CONNECTING;
//   this->connection_start_ = std::chrono::steady_clock::now();
// 
//   auto scope = sp.create_scope("Client");
//   imt_service::enable_async(scope);
//   tcp_network_socket::connect(
//     sp,
//     this->address_,
//     this->port_)
//     .then(
//       [this](
//         const std::function<void(const service_provider & sp)> & done,
//         const error_handler & on_error,
//         const service_provider & sp,
//         const tcp_network_socket & s) {
// 
//     sp.get<logger>()->debug("client", sp, "Connected");
//     auto client_crypto_tunnel = std::make_shared<ssl_tunnel>(true, this->client_certificate_, this->client_private_key_);
// 
//     async_series(
//       
//       this->client_.start(
//         sp,
//         client_crypto_tunnel->decrypted_output(), client_crypto_tunnel->decrypted_input(),
//         [sp, this](const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {
//             if (!request) {
//               return this->incoming_stream_->write_async(sp, nullptr, 0);
//             }
//             else {
//               return this->incoming_stream_->write_value_async(sp, request);
//             }
//           }),
//                    
//       copy_stream(
//         sp,
//         s.incoming(),
//         client_crypto_tunnel->crypted_input()
//       ),
//       copy_stream(
//         sp,
//         client_crypto_tunnel->crypted_output(),
//         s.outgoing()
//       ),
//       dataflow(
//         stream_read<continuous_buffer<std::shared_ptr<json_value>>>(this->outgoing_stream_),
//         json_to_http_channel("POST", "/vds/client_api"),
//         stream_write(this->client_.output_commands())
//       )
//     ).wait(
//         [this, client_crypto_tunnel](const service_provider & sp) {
//       sp.get<logger>()->debug("client", sp, "Client closed");
// 
//       std::unique_lock<std::mutex> lock(this->send_state_mutex_);
//       this->state_ = STATE::NONE;
//       this->state_cond_.notify_all();
//     },
//         [this, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
//       sp.get<logger>()->debug("client", sp, "Client error");
//       std::unique_lock<std::mutex> lock(this->send_state_mutex_);
//       this->state_ = STATE::CONNECT_ERROR;
//       this->state_cond_.notify_all();
//       on_error(sp, ex);
//     },
//       sp.create_scope("Client dataflow"));
//     client_crypto_tunnel->start(sp);
// 
//     done(sp);
// 
//   })
//     .wait(
//       [this](const service_provider & sp) {
//         std::unique_lock<std::mutex> lock(this->send_state_mutex_);
//         this->state_ = STATE::CONNECTED;
//         this->state_cond_.notify_all();
//         this->connection_start_ = std::chrono::steady_clock::now();
//       },
//       [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
//         std::unique_lock<std::mutex> lock(this->send_state_mutex_);
//         this->state_ = STATE::CONNECT_ERROR;
//         this->state_cond_.notify_all();
//         sp.unhandled_exception(ex);
//       },
//       scope);
}

void vds::client_connection::stop(const service_provider & sp)
{
  for (;;) {
    std::unique_lock<std::mutex> lock(this->state_mutex_);
    switch (this->state_) {
    case STATE::CONNECTING:
    case STATE::CONNECTED:
      this->state_cond_.wait_for(lock, std::chrono::seconds(5));
      break;
    default:
      return;
    }
  }
}
