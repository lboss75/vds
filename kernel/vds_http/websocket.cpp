/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "websocket.h"

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::websocket::open_connection(
  const vds::service_provider * sp,
  const std::shared_ptr<http_async_serializer> & output_stream,
  const http_message & request,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, bool /*is_binary*/, std::shared_ptr<websocket_output>> handler)
{
	std::string websocket_key;
	if (request.get_header("Sec-WebSocket-Key", websocket_key))
	{
		websocket_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
		GET_EXPECTED_ASYNC(result_hash, hash::signature(hash::sha1(), const_data_buffer(websocket_key.c_str(), websocket_key.length())));

		std::list<std::string> headers;
		headers.push_back("HTTP/1.1 101 Switching Protocols");
		headers.push_back("Upgrade: websocket");
		headers.push_back("Connection: Upgrade");
		headers.push_back("Sec-WebSocket-Accept: " + base64::from_bytes(result_hash));
		//headers.push_back("Sec-WebSocket-Protocol: chat");

    GET_EXPECTED_ASYNC(output, co_await output_stream->start_message(headers));

		co_return std::make_shared<websocket_handler>(sp, output, std::move(handler));
	}

  CHECK_EXPECTED_ASYNC(co_await http_response::status_response(output_stream, http_response::HTTP_Not_Found, "Invalid protocol"));

  co_return std::shared_ptr<vds::stream_output_async<uint8_t>>();
}

vds::websocket::websocket_handler::websocket_handler(
  const vds::service_provider * sp,
  std::shared_ptr<stream_output_async<uint8_t>> target,
  lambda_holder_t<async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>>, bool /*is_binary*/, std::shared_ptr<websocket_output>> handler)
  : sp_(sp), target_(std::make_shared<websocket_output>(target)), handler_(std::move(handler)) {
}

vds::async_task<vds::expected<void>> vds::websocket::websocket_handler::write_async(const uint8_t * data, size_t len)
{
  if (0 == len) {
    //TODO
  }
  else {
    while (len > 0) {
      switch (this->read_state_) {
      case read_state_t::HEADER: {
        if (!this->read_minimal(3, data, len)) {
          co_return expected<void>();
        }

        this->fin_ = (0x80 == (this->buffer_[0] & 0x80));
        this->RSV1_ = (0x40 == (this->buffer_[0] & 0x40));
        this->RSV2_ = (0x20 == (this->buffer_[0] & 0x20));
        this->RSV3_ = (0x10 == (this->buffer_[0] & 0x10));
        this->Opcode_ = (this->buffer_[0] & 0x0F);

        this->has_mask_ = (0x80 == (this->buffer_[1] & 0x80));
        this->payloadLength_ = (this->buffer_[1] & 0x7F);

        uint8_t offset;
        switch (this->payloadLength_) {
        case 126: {
          if (!this->read_minimal(4 + (this->has_mask_ ? 4 : 0), data, len)) {
            co_return expected<void>();
          }

          this->payloadLength_ = (uint64_t)this->buffer_[2] << 8 | this->buffer_[3];
          offset = 4;
          break;
        }
        case 127: {
          if (!this->read_minimal(10 + (this->has_mask_ ? 4 : 0), data, len)) {
            co_return expected<void>();
          }

          this->payloadLength_ =
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
          if (!this->read_minimal(2 + (this->has_mask_ ? 4 : 0), data, len)) {
            co_return expected<void>();
          }

          offset = 2;
          break;
        }
        }

        if (this->has_mask_) {
          memcpy(this->mask_, this->buffer_ + offset, 4);
          this->mask_index_ = 0;
        }

        switch (this->Opcode_) {
          //case 0x0://continuation frame
          //	break;

        case 0x1://text frame
          this->read_state_ = read_state_t::TEXT;
          break;

        case 0x2://binary frame
          this->read_state_ = read_state_t::BINARY;
          break;

        case 0x8: {//connection close
          co_return make_unexpected<std::runtime_error>("Not implemented");//TODO
          //uint16_t error_code = ((uint16_t)data[0] << 8) | data[1];
          //auto error_message = utf16::from_utf8(std::string((char *)data + 2, this->payloadLength_ - 2));
          //this->read_state_ = read_state_t::CLOSED;
        }

                  //case 0x9://ping
                  //	break;

                  //case 0xA://pong
                  //	break;

        default:
          this->read_state_ = read_state_t::CLOSED;
          co_return make_unexpected<std::runtime_error>("Not implemented");//TODO
        }

        GET_EXPECTED_VALUE_ASYNC(
          this->current_stream_,
          co_await this->handler_(this->read_state_ == read_state_t::BINARY, this->target_));

        break;
      }

      case read_state_t::TEXT:
      case read_state_t::BINARY: {
        auto size = len;
        if (size > this->payloadLength_) {
          size = safe_cast<decltype(size)>(this->payloadLength_);
        }

        if (this->has_mask_) {
          if (size > sizeof(this->buffer_) / sizeof(this->buffer_[0])) {
            size = sizeof(this->buffer_) / sizeof(this->buffer_[0]);
          }

          memcpy(this->buffer_, data, size);

          for (decltype(size) i = 0; i < size;) {
            this->buffer_[i++] ^= this->mask_[this->mask_index_++ % 4];
          }

          CHECK_EXPECTED_ASYNC(co_await this->current_stream_->write_async(this->buffer_, size));
        }
        else {
          CHECK_EXPECTED_ASYNC(co_await this->current_stream_->write_async(data, size));
        }

        data += size;
        len -= size;
        this->payloadLength_ -= size;
        this->readed_ = 0;
        if (0 == this->payloadLength_) {
          CHECK_EXPECTED_ASYNC(co_await this->current_stream_->write_async(nullptr, 0));
          this->current_stream_.reset();
          this->read_state_ = read_state_t::HEADER;
        }

        break;
      }

      default:
        co_return make_unexpected<std::runtime_error>("Not implemented");//TODO
      }
    }
  }

  co_return expected<void>();
}

bool vds::websocket::websocket_handler::read_minimal(uint8_t min_size, const uint8_t * & data, size_t & len)
{
	if(this->readed_ < min_size) {
    auto l = len;
    if (l > (decltype(l))(min_size - this->readed_)) {
      l = min_size - this->readed_;
    }
    memcpy(this->buffer_ + this->readed_, data, l);

    this->readed_ += safe_cast<decltype(this->readed_)>(l);
    data += l;
    len -= l;
	}

	return (this->readed_ >= min_size);
}

vds::async_task<vds::expected<std::shared_ptr<vds::stream_output_async<uint8_t>>>> vds::websocket_output::start(uint64_t message_size, bool is_binary)
{
  uint8_t buffer[10];

  buffer[0] = (is_binary ? 0x82 : 0x81);

  uint8_t offset;
  if (message_size < 126)
  {
    buffer[1] = (uint8_t)message_size;
    offset = 2;
  }
  else if (message_size <= 0xFFFF)
  {
    buffer[1] = 126;
    buffer[2] = (uint8_t)((message_size >> 8) & 0xFF);
    buffer[3] = (uint8_t)(message_size & 0xFF);
    offset = 4;
  }
  else
  {
    buffer[1] = 127;
    buffer[2] = (uint8_t)((message_size >> 56) & 0xFF);
    buffer[3] = (uint8_t)((message_size >> 48) & 0xFF);
    buffer[4] = (uint8_t)((message_size >> 40) & 0xFF);
    buffer[5] = (uint8_t)((message_size >> 32) & 0xFF);
    buffer[6] = (uint8_t)((message_size >> 24) & 0xFF);
    buffer[7] = (uint8_t)((message_size >> 16) & 0xFF);
    buffer[8] = (uint8_t)((message_size >> 8) & 0xFF);
    buffer[9] = (uint8_t)(message_size & 0xFF);
    offset = 10;
  }

  co_await this->async_mutex_.lock();

  CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(buffer, offset));
  co_return std::make_shared<output_stream>(this->shared_from_this(), message_size);
}

vds::async_task<vds::expected<void>> vds::websocket_output::output_stream::write_async(const uint8_t * data, size_t len)
{
  vds_assert(this->message_size_ >= len);
  this->message_size_ -= len;
  CHECK_EXPECTED_ASYNC(co_await this->target_->target_->write_async(data, len));

  if (0 == this->message_size_) {
    this->target_->async_mutex_.unlock();
  }

  co_return expected<void>();
}
