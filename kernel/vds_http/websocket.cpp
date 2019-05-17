/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "websocket.h"

vds::async_task<vds::expected<vds::http_message>> vds::websocket::open_connection(const vds::service_provider * sp, const http_request & request)
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

		//auto handler = std::make_shared<websocket_api>(sp);

		//co_return http_message(headers, handler->start(request));
	}

	co_return http_response::status_response(http_response::HTTP_Not_Found, "Invalid protocol");
}

vds::websocket::websocket(const service_provider * sp, const std::function<async_task<expected<void>>(message msg, std::shared_ptr<websocket_output> & out)> & handler)
	: sp_(sp), handler_(handler)
{
}

std::shared_ptr<vds::stream_input_async<uint8_t>> vds::websocket::start(const http_request & request)
{
	throw vds::vds_exceptions::invalid_operation();
}

vds::async_task<vds::expected<bool>> vds::websocket::read_minimal(size_t min_size)
{
	while (this->readed_ < min_size) {
		GET_EXPECTED_ASYNC(len, co_await this->input_stream_->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_));

		if (0 == len) {
			co_return false;
		}

		this->readed_ += len;
	}

	co_return true;
}


vds::async_task<vds::expected<vds::websocket::message_type>> vds::websocket::next_message()
{
	for (;;) {
		GET_EXPECTED_ASYNC(isOk, co_await this->read_minimal(3));
		if (!isOk) {
			this->read_state_ = read_state_t::CLOSED;
			if (0 == this->readed_) {
				co_return message_type::close;
			}
			else {
				co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
			}
		}

		this->fin_ = (0x80 == (this->buffer_[0] & 0x80));
		this->RSV1_ = (0x40 == (this->buffer_[0] & 0x40));
		this->RSV2_ = (0x20 == (this->buffer_[0] & 0x20));
		this->RSV3_ = (0x10 == (this->buffer_[0] & 0x10));
		this->Opcode_ = (this->buffer_[0] & 0x0F);

		this->has_mask_ = (0x80 == (this->buffer_[1] & 0x80));
		this->payloadLength_ = (this->buffer_[1] & 0x7F);
		this->offset_;

		if (this->has_mask_) {
			memcpy(this->mask_, this->buffer_ + this->offset_, 4);
			this->mask_index_ = 0;
			this->offset_ += 4;
		}

		switch (this->payloadLength_) {
		case 126: {
			GET_EXPECTED_VALUE_ASYNC(isOk, co_await this->read_minimal(4));
			if (!isOk) {
				this->read_state_ = read_state_t::CLOSED;
				if (0 == this->readed_) {
					co_return message_type::close;
				}
				else {
					co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
				}
			}

			this->payloadLength_ = (uint64_t)this->buffer_[2] << 8 | this->buffer_[3];
			this->offset_ = 4;
			break;
		}
		case 127: {
			GET_EXPECTED_VALUE_ASYNC(isOk, co_await this->read_minimal(10));
			if (!isOk) {
				this->read_state_ = read_state_t::CLOSED;
				if (0 == this->readed_) {
					co_return message_type::close;
				}
				else {
					co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
				}
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
			this->offset_ = 10;
			break;
		}
		default: {
			this->offset_ = 2;
			break;
		}
		}

		switch (this->Opcode_) {
			//case 0x0://continuation frame
			//	break;

		case 0x1://text frame
			this->read_state_ = read_state_t::TEXT;
			co_return message_type::text;

		case 0x2://binary frame
			this->read_state_ = read_state_t::BINARY;
			co_return message_type::binary;

		case 0x8: {//connection close
			uint16_t error_code = ((uint16_t)this->buffer_[this->offset_] << 8) | this->buffer_[this->offset_ + 1];
			this->offset_ += 2;
			auto error_message = utf16::from_utf8(std::string((char *)this->buffer_ + this->offset_, this->payloadLength_ - 2));
			this->read_state_ = read_state_t::CLOSED;
			co_return message_type::close;
		}

				  //case 0x9://ping
				  //	break;

				  //case 0xA://pong
				  //	break;

		default:
			this->read_state_ = read_state_t::CLOSED;
			co_return message_type::close;
		}
	}
}

vds::async_task<vds::expected<size_t>> vds::websocket::read(uint8_t * buffer, size_t buffer_size)
{
	if (this->payloadLength_ > 0) {
		if (0 == this->readed_) {
			this->offset_ = 0;
			GET_EXPECTED_VALUE_ASYNC(this->readed_, co_await this->input_stream_->read_async(this->buffer_, sizeof(this->buffer_)));

			if (0 == this->readed_) {
				co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
			}
		}


		size_t len = this->readed_ - this->offset_;
		if (len > this->payloadLength_) {
			len = this->payloadLength_;
		}
		
		if (len > buffer_size) {
			len = buffer_size;
		}

		if (this->has_mask_) {
			for (int i = 0; i < len; ++i) {
				this->buffer_[this->offset_ + i] ^= this->mask_[this->mask_index_++ % 4];
			}
		}

		memcpy(buffer, this->buffer_ + this->offset_, len);

		this->payloadLength_ -= len;
		this->readed_ -= len;

		if (0 < this->readed_) {
			this->offset_ += len;
		}

		co_return len;
	}
	else {
		co_return 0;
	}
}


vds::async_task<vds::expected<void>> vds::websocket::start_read()
{
	for (;;) {
		GET_EXPECTED_ASYNC(type, co_await this->next_message());

		if (message_type::close == type) {
			break;
		}

		auto out_stream = std::make_shared<websocket_output>();
		CHECK_EXPECTED_ASYNC(co_await this->handler_(message{ type, std::make_shared<websocket_stream>(this->shared_from_this()) }, out_stream));


	}
}

