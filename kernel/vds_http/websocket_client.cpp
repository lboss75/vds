/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "websocket_client.h"
#include "network_address.h"
#include "url_parser.h"
#include "tcp_network_socket.h"

vds::async_task<vds::expected<void>> vds::websocket_client::open_connection(
  const service_provider* sp,
  const std::string& url)
{
  GET_EXPECTED(url_address, url_parser::parse_network_address(url));
  GET_EXPECTED(address, network_address::parse_server_address(url_address.server + ":" + url_address.port));
  GET_EXPECTED(s, tcp_network_socket::connect(sp, address));
  GET_EXPECTED(writer, s->get_output_stream(sp));

  this->client_ = std::make_shared<http_client>();
  CHECK_EXPECTED(this->client_->start(sp, s, writer));

  uint8_t key_buffer[6];
  for (size_t i = 0; i < sizeof(key_buffer); ++i) {
    key_buffer[i] = (uint8_t)std::rand();
  }


  std::list<std::string> headers;
  headers.push_back("GET " + url_address.path + " HTTP/1.1");
  headers.push_back("Upgrade: websocket");
  headers.push_back("Connection: Upgrade");
  headers.push_back("Sec-WebSocket-Key: " + base64::from_bytes(key_buffer, sizeof(key_buffer)));
  headers.push_back("Sec-WebSocket-Protocol: chat, superchat");
  headers.push_back("Sec-WebSocket-Version: 7");

  return this->client_->send_headers(
    http_message(headers),
    [pthis = this->shared_from_this()](http_message message)->async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> {
      http_response response(message);
      if (http_response::HTTP_OK != response.code()) {
        return vds::make_unexpected<std::runtime_error>("Connection error " + response.comment());
      }

      return std::make_shared<input_stream>(pthis);
    },
    [pthis = this->shared_from_this()](std::shared_ptr<stream_output_async<uint8_t>> body_stream)->async_task<expected<void>> {
      pthis->body_stream_ = body_stream;
      return pthis->body_result_.get_future();
    });
}

vds::async_task<vds::expected<void>> vds::websocket_client::send_message(const std::string& message)
{
  return this->send_message(reinterpret_cast<const uint8_t *>(message.c_str()), message.length(), false);
}

vds::async_task<vds::expected<void>> vds::websocket_client::send_message(const uint8_t* message, uint64_t message_size, bool is_binary)
{
  this->send_buffer_.clear();
  CHECK_EXPECTED(this->send_buffer_.add(is_binary ? 0x82 : 0x81));

  if (message_size < 126)
  {
    CHECK_EXPECTED(this->send_buffer_.add(0x80 | (uint8_t)message_size));
  }
  else if (message_size <= 0xFFFF)
  {
    CHECK_EXPECTED(this->send_buffer_.add(0x80 | 126));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 8) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)(message_size & 0xFF)));
  }
  else
  {
    CHECK_EXPECTED(this->send_buffer_.add(0x80 | 127));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 56) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 48) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 40) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 32) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 24) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 16) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)((message_size >> 8) & 0xFF)));
    CHECK_EXPECTED(this->send_buffer_.add((uint8_t)(message_size & 0xFF)));
  }

  uint8_t mask[4] = {
    (uint8_t)std::rand(),
    (uint8_t)std::rand(),
    (uint8_t)std::rand(),
    (uint8_t)std::rand()
  };

  for (int i = 0; i < sizeof(mask); ++i) {
    CHECK_EXPECTED(this->send_buffer_.add(mask[i]));
  }

  uint8_t index = 0;
  while (0 < message_size--) {
    CHECK_EXPECTED(this->send_buffer_.add(mask[3 & index++] ^ *message++));
  }

  return this->body_stream_->write_async(this->send_buffer_.data(), this->send_buffer_.size());
}

vds::websocket_client::input_stream::input_stream(std::shared_ptr<websocket_client> owner)
: owner_(owner) {
}

vds::async_task<vds::expected<void>> vds::websocket_client::input_stream::write_async(
  const uint8_t* data,
  size_t len)
{
  if (0 == len) {
    if (0 != this->buffer_.size()) {
      co_return make_unexpected<std::runtime_error>("Unexpected end of stream");
    }
    co_return expected<void>();
  }

  CHECK_EXPECTED(this->buffer_.add(data, len));
  if (2 > this->buffer_.size()) {
    co_return expected<void>();
  }

  bool fin = (0x80 == (this->buffer_[0] & 0x80));
  int opcode = (this->buffer_[0] & 0x0F);

  bool has_mask = (0x80 == (this->buffer_[1] & 0x80));
  uint64_t payloadLength = (this->buffer_[1] & 0x7F);

  size_t offset;
  switch (payloadLength) {
  case 126: {
    if ((4u + (has_mask ? 4u : 0u)) > this->buffer_.size()) {
      co_return expected<void>();
    }

    payloadLength = (uint64_t)this->buffer_[2] << 8 | this->buffer_[3];
    offset = 4;
    break;
  }
  case 127: {
    if ((10u + (has_mask ? 4u : 0u)) > this->buffer_.size()) {
      co_return expected<void>();
    }

    payloadLength =
      ((uint64_t)this->buffer_[2] << 56) |
      ((uint64_t)this->buffer_[3] << 48) |
      ((uint64_t)this->buffer_[4] << 40) |
      ((uint64_t)this->buffer_[5] << 32) |
      ((uint64_t)this->buffer_[6] << 24) |
      ((uint64_t)this->buffer_[7] << 16) |
      ((uint64_t)this->buffer_[8] << 8) |
      this->buffer_[9];
    offset = 10;
    break;
  }
  default: {
    if ((2u + (has_mask ? 4u : 0u)) > this->buffer_.size()) {
      co_return expected<void>();
    }

    offset = 2;
    break;
  }
  }
  if ((payloadLength + offset + (has_mask ? 4u : 0u)) > this->buffer_.size()) {
    co_return expected<void>();
  }

  if (has_mask) {
    uint8_t index = 0;
    auto p = this->buffer_.data() + offset + 4u;
    auto data_len = payloadLength;
    while (0u < data_len--) {
      *p++ ^= this->buffer_[offset + (3u & index++)];
    }
  }

  switch (opcode)
  {
  case 0x1://text frame
    CHECK_EXPECTED_ASYNC(co_await this->owner_->text_handler_(
      std::string(
        reinterpret_cast<const char *>(this->buffer_.data() + offset + (has_mask ? 4 : 0)),
        safe_cast<size_t>(payloadLength))));
    break;

  case 0x2://binary frame
    CHECK_EXPECTED_ASYNC(co_await this->owner_->binary_handler_(
      const_data_buffer(
        this->buffer_.data() + offset + (has_mask ? 4 : 0),
        safe_cast<size_t>(payloadLength))));
    break;

  default:
    co_return make_unexpected<std::runtime_error>("Unexpected opcode " + std::to_string(opcode));
  }

  this->buffer_.remove(0, safe_cast<size_t>(payloadLength + offset + (has_mask ? 4 : 0)));
  co_return expected<void>();
}
