/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "websocket_api.h"

vds::async_task<vds::expected<vds::http_message>> vds::websocket_api::open_connection(const vds::service_provider * sp, const http_request & request)
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

		auto handler = std::make_shared<websocket_api>(sp);

		co_return http_message(headers, handler->start(request));
	}

	co_return http_response::status_response(http_response::HTTP_Not_Found, "Invalid protocol");
}

vds::websocket_api::websocket_api(const service_provider * sp)
	: sp_(sp)
{
}

std::shared_ptr<vds::stream_input_async<uint8_t>> vds::websocket_api::start(const http_request & request)
{
	auto buffer = std::make_shared<continuous_buffer<uint8_t>>(this->sp_);

	json_parser::options options;
	options.enable_multi_root_objects = true;

	auto parser = std::make_shared<json_parser>("Web Socket", [pthis = this->shared_from_this(), buffer](const std::shared_ptr<json_value> & message)->expected<void> {

		auto request = std::dynamic_pointer_cast<json_object>(message);
		if (request) {
			int id;
			GET_EXPECTED(isOk, request->get_property("id", id));

			if (isOk) {
					
			}
		}

		static const char error_message[] = "{\"error\": \"Invalid command\"}";
		return buffer->write_async((const uint8_t *)error_message, sizeof(error_message)).get();
	}, options);

	this->task_ = this->start_parse(parser, request.get_message().body());

	return std::make_shared<continuous_stream_input_async<uint8_t>>(buffer);
}

vds::async_task<vds::expected<void>> vds::websocket_api::start_parse(
	std::shared_ptr<vds::json_parser> parser,
	std::shared_ptr<vds::stream_input_async<uint8_t>> request_stream)
{
	uint8_t buffer[1024];
	size_t readed = 0;
	for (;;) {
		while (readed < 3) {
			GET_EXPECTED_ASYNC(len, co_await request_stream->read_async(buffer + readed, sizeof(buffer) - readed));

			if (0 == len) {
				if (0 == readed) {
					CHECK_EXPECTED_ASYNC(parser->write(nullptr, 0));
					co_return expected<void>();
				}
				else {
					co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
				}
			}

			readed += len;
		}

		bool fin = (0x80 == (buffer[0] & 0x80));
		bool RSV1 = (0x40 == (buffer[0] & 0x40));
		bool RSV2 = (0x20 == (buffer[0] & 0x20));
		bool RSV3 = (0x10 == (buffer[0] & 0x10));
		unsigned int Opcode = (buffer[0] & 0x0F);

		bool has_mask = (0x80 == (buffer[1] & 0x80));
		uint64_t payloadLength = (buffer[1] & 0x7F);
		size_t offset;

		switch (payloadLength) {
		case 126: {
			while (readed < 4) {
				GET_EXPECTED_ASYNC(len, co_await request_stream->read_async(buffer + readed, sizeof(buffer) - readed));

				if (0 == len) {
					if (0 == readed) {
						CHECK_EXPECTED_ASYNC(parser->write(nullptr, 0));
						co_return expected<void>();
					}
					else {
						co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
					}
				}

				readed += len;
			}

			payloadLength = (uint64_t)buffer[2] << 8 | buffer[3];
			offset = 4;
			break;
		}
		case 127: {
			while (readed < 10) {
				GET_EXPECTED_ASYNC(len, co_await request_stream->read_async(buffer + readed, sizeof(buffer) - readed));

				if (0 == len) {
					if (0 == readed) {
						CHECK_EXPECTED_ASYNC(parser->write(nullptr, 0));
						co_return expected<void>();
					}
					else {
						co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
					}
				}

				readed += len;
			}

			payloadLength =
				((uint64_t)buffer[2] << 56) |
				((uint64_t)buffer[3] << 48) |
				((uint64_t)buffer[4] << 40) |
				((uint64_t)buffer[5] << 32) |
				((uint64_t)buffer[6] << 24) |
				((uint64_t)buffer[7] << 16) |
				((uint64_t)buffer[8] << 8) |
				buffer[9];
			offset = 10;
			break;
		}
		default: {
			offset = 2;
			break;
		}
		}

		switch (Opcode) {
			//case 0x0://continuation frame
			//	break;

		case 0x1://text frame
			break;

			//case 0x2://binary frame
			//	break;

		case 0x8: {//connection close
			uint16_t error_code = ((uint16_t)buffer[offset] << 8) | buffer[offset + 1];
			offset += 2;
			auto error_message = utf16::from_utf8(std::string((char *)buffer + offset, payloadLength - 2));
			CHECK_EXPECTED_ASYNC(parser->write(nullptr, 0));
			co_return expected<void>();
			break;
		}
					//case 0x9://ping
					//	break;

					//case 0xA://pong
					//	break;

		default:
			CHECK_EXPECTED_ASYNC(parser->write(nullptr, 0));
			co_return expected<void>();
			break;
		}

		uint8_t mask[4];
		int mask_index = 0;
		if (has_mask) {
			memcpy(mask, buffer + offset, 4);
			offset += 4;
		}

		while (payloadLength > 0) {
			size_t len = readed - offset;
			if (len > payloadLength) {
				len = payloadLength;
			}

			if (has_mask) {
				for (int i = 0; i < len; ++i) {
					buffer[offset + i] ^= mask[mask_index++ % 4];
				}
			}

			CHECK_EXPECTED_ASYNC(parser->write(buffer + offset, len));

			payloadLength -= len;
			readed -= len;

			if (0 < readed) {
				memmove(buffer, buffer + offset + len, readed);
				offset = 0;
			}

			if (0 == payloadLength) {
				break;
			}
			else {
				if (0 == readed) {
					GET_EXPECTED_ASYNC(readed, co_await request_stream->read_async(buffer, sizeof(buffer)));

					if (0 == readed) {
						co_return make_unexpected<std::runtime_error>("Invalid web socket protocol");
					}
				}
			}
		}
	}
}


