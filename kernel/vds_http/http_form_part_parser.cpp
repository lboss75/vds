/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_form_part_parser.h"

//vds::async_task<vds::expected<void>> vds::http_form_part_parser::write_async(const uint8_t * data, size_t len) {
//	vds_assert(this->state_ != parse_state::CLOSED);
//
//	if (0 == len) {
//		logger::get(this->sp_)->debug("HTTP", "HTTP end");
//		if(this->state_ == parse_state::BODY) {
//			CHECK_EXPECTED_ASYNC(co_await this->current_message_body_->write_async(nullptr, 0));
//			this->state_ = parse_state::CLOSED;
//		}
//		co_return expected<void>();
//	}
//	else {
//		if (this->state_ == parse_state::BODY) {
//			CHECK_EXPECTED_ASYNC(co_await this->current_message_body_->write_async(data, len));
//			co_return expected<void>();
//		}
//
//		while (len > 0) {
//			char *p = (char *)memchr((const char *)data, '\n', len);
//			if (nullptr == p) {
//				this->parse_buffer_ += std::string((const char *)data, len);
//				len = 0;
//				break;
//			}
//
//			auto size = p - (const char *)data;
//
//			if (size > 0) {
//				if ('\r' == reinterpret_cast<const char *>(data)[size - 1]) {
//					this->parse_buffer_ += std::string((const char *)data, size - 1);
//				}
//				else {
//					this->parse_buffer_.append((const char *)data, size);
//				}
//			}
//
//			data += size + 1;
//			len -= size + 1;
//
//			if (0 == this->parse_buffer_.length()) {
//				http_message message(this->headers_);
//				this->headers_.clear();
//
//				GET_EXPECTED_ASYNC(target_stream, co_await this->message_callback_(message));
//
//				this->current_message_body_ = std::make_shared<message_body_reader>(this->shared_from_this(), data, len);
//
//
//				static_cast<message_body_reader *>(this->current_message_body_.get())->get_rest_data(this->buffer_, len);
//				data = this->buffer_;
//			}
//			else {
//				this->headers_.push_back(this->parse_buffer_);
//				this->parse_buffer_.clear();
//			}
//		}
//	}
//}
