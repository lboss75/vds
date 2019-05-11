/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "json_parser.h"

vds::expected<void> vds::json_parser::start_array() {
	auto new_object = std::make_shared<json_array>(this->line_, this->column_);

	switch (this->state_) {
	case ST_OBJECT_PROPERTY_VALUE:
		std::static_pointer_cast<json_property>(this->current_object_)->value(new_object);
		break;

	case ST_BOF:
		this->saved_states_.push(ST_BOF);
		break;

	case ST_ARRAY:
		std::static_pointer_cast<json_array>(this->current_object_)->add(new_object);
		this->saved_states_.push(ST_ARRAY_ITEM);
		this->current_path_.push(this->current_object_);
		break;

	default:
		return vds::make_unexpected<parse_error>(
			this->stream_name_,
			this->line_,
			this->column_,
			"Unexpected state");
	}

	this->current_object_ = new_object;
	this->state_ = ST_ARRAY;
	return expected<void>();
}

vds::expected<void> vds::json_parser::final_array() {
	this->state_ = this->saved_states_.top();
	this->saved_states_.pop();

	if (ST_BOF == this->state_) {
		if (!this->parse_options_.enable_multi_root_objects) {
			this->state_ = ST_EOF;
		}

		return this->result_(this->root_object_);
	}
	else {
		this->current_object_ = this->current_path_.top();
		this->current_path_.pop();
		return expected<void>();
	}
}

vds::json_parser::json_parser(const std::string &stream_name,
	const std::function<expected<void>(const std::shared_ptr<json_value> &)> &result,
	const json_parser::options &parse_options)
	: stream_name_(stream_name),
	result_(result),
	parse_options_(parse_options),
	state_(ST_BOF),
	current_object_(nullptr),
	line_(1), column_(1) {

}

vds::expected<void> vds::json_parser::write(const uint8_t * input_buffer, size_t input_len) {
	if (nullptr == input_buffer || 0 == input_len) {
		return this->final_data();
	}

	while (0 < input_len--) {
		auto current_symbol = (char)*input_buffer++;
		switch (current_symbol) {
		case '\n':
			this->line_++;
			this->column_ = 1;
			break;

		case '\t':
			this->column_ = 1 + ((this->column_ + 7) & ~7);
			break;

		default:
			this->column_++;
			break;
		}

		switch (this->state_) {
		case ST_BOF:
			switch (current_symbol) {
			case '/':
				CHECK_EXPECTED(this->after_slesh());
				break;

			case '[':
				CHECK_EXPECTED(this->start_array());
				this->root_object_ = this->current_object_;
				break;

			case '{':
				CHECK_EXPECTED(this->start_object());
				this->root_object_ = this->current_object_;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;
		case ST_AFTER_SLESH:
			switch (current_symbol) {
			case '/':
				this->state_ = ST_INLINE_COMMENT;
				break;
			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;
		case ST_INLINE_COMMENT:
			switch (current_symbol) {
			case '\n':
				this->state_ = this->saved_states_.top();
				saved_states_.pop();
				break;
			default:
				break;
			};
			break;
		case ST_ARRAY:
			switch (current_symbol) {
			case ']':
				CHECK_EXPECTED(this->final_array());
				break;

			case '[':
				CHECK_EXPECTED(this->start_array());
				break;

			case '{':
				CHECK_EXPECTED(this->start_object());
				break;

			case '/':
				CHECK_EXPECTED(this->after_slesh());
				break;

			case '\"':
				this->saved_states_.push(ST_ARRAY_ITEM);
				this->state_ = ST_STRING;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				if (isdigit(current_symbol)) {
					this->buffer_ = current_symbol;
					this->saved_states_.push(ST_ARRAY_ITEM);
					this->state_ = ST_NUMBER;
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			};
			break;

		case ST_ARRAY_ITEM:
			switch (current_symbol) {
			case ']':
				CHECK_EXPECTED(this->final_array());
				break;

			case ',':
				this->state_ = ST_ARRAY;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_OBJECT:
			switch (current_symbol) {
			case '\"':
				this->saved_states_.push(ST_OBJECT_ITEM);
				this->saved_states_.push(ST_OBJECT_PROPERTY_NAME);
				this->state_ = ST_STRING;
				break;

			case '}':
				CHECK_EXPECTED(this->final_object());
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_OBJECT_ITEM:
			switch (current_symbol) {
			case '}':
				CHECK_EXPECTED(this->final_object());
				break;

			case ',':
				this->state_ = ST_OBJECT;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_INTEGER:
			switch (current_symbol) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->buffer_ += current_symbol;
				break;
			default:
				this->state_ = this->saved_states_.top();
				saved_states_.pop();
				switch (this->state_) {
				case ST_OBJECT_ITEM:
					this->final_integer_property();
					break;

				case ST_ARRAY_ITEM:
					std::static_pointer_cast<json_array>(this->current_object_)->add(
						std::make_shared<json_primitive>(
							this->line_, this->column_,
							this->buffer_,
							json_primitive::primitive_type::integer
							));
					this->buffer_.clear();
					break;

				default:
					return vds::make_unexpected<parse_error>(
						this->stream_name_,
						this->line_,
						this->column_,
						std::string("Unexpected char ") + current_symbol);
				}

				//Step back
				++input_len;
				--input_buffer;
				switch (current_symbol) {
				case '\n':
					this->line_--;
					break;

				default:
					this->column_--;
					break;
				}

				break;
			}
			break;

		case ST_STRING:
			switch (current_symbol) {
			case '\\':
				this->state_ = ST_STRING_BACKSLESH;
				break;
			case '\"':
				this->state_ = this->saved_states_.top();
				saved_states_.pop();
				switch (this->state_) {
				case ST_OBJECT_PROPERTY_NAME:
					this->start_property();
					break;

				case ST_OBJECT_ITEM:
					this->final_string_property();
					break;

				case ST_ARRAY_ITEM:
					std::static_pointer_cast<json_array>(this->current_object_)->add(
						std::make_shared<json_primitive>(
							this->line_, this->column_,
							this->buffer_

							));
					this->buffer_.clear();
					break;

				default:
					return vds::make_unexpected<parse_error>(
						this->stream_name_,
						this->line_,
						this->column_,
						std::string("Unexpected char ") + current_symbol);
				}
				break;
			default:
				this->buffer_ += current_symbol;
				break;
			}
			break;

		case ST_STRING_BACKSLESH:
			switch (current_symbol) {
			case '\"':
			case '\\':
				this->buffer_ += current_symbol;
				this->state_ = ST_STRING;
				break;

			case 'n':
				this->buffer_ += '\n';
				this->state_ = ST_STRING;
				break;

			case 'r':
				this->buffer_ += '\r';
				this->state_ = ST_STRING;
				break;

			case 'b':
				this->buffer_ += '\b';
				this->state_ = ST_STRING;
				break;

			case 't':
				this->buffer_ += '\t';
				this->state_ = ST_STRING;
				break;

			case 'f':
				this->buffer_ += '\f';
				this->state_ = ST_STRING;
				break;

			case 'u':
				this->state_ = ST_STRING_SYMBOL_1;
				break;

			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_STRING_SYMBOL_1:
			switch (current_symbol) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->num_buffer_ = current_symbol - '0';
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'a':
			case 'A':
				this->num_buffer_ = 10;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'b':
			case 'B':
				this->num_buffer_ = 11;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'c':
			case 'C':
				this->num_buffer_ = 12;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'd':
			case 'D':
				this->num_buffer_ = 13;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'e':
			case 'E':
				this->num_buffer_ = 14;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			case 'f':
			case 'F':
				this->num_buffer_ = 15;
				this->state_ = ST_STRING_SYMBOL_2;
				break;

			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_STRING_SYMBOL_2:
			this->num_buffer_ <<= 4;
			switch (current_symbol) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->num_buffer_ |= current_symbol - '0';
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'a':
			case 'A':
				this->num_buffer_ |= 10;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'b':
			case 'B':
				this->num_buffer_ |= 11;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'c':
			case 'C':
				this->num_buffer_ |= 12;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'd':
			case 'D':
				this->num_buffer_ |= 13;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'e':
			case 'E':
				this->num_buffer_ |= 14;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			case 'f':
			case 'F':
				this->num_buffer_ |= 15;
				this->state_ = ST_STRING_SYMBOL_3;
				break;

			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;


		case ST_STRING_SYMBOL_3:
			this->num_buffer_ <<= 4;
			switch (current_symbol) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->num_buffer_ |= current_symbol - '0';
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'a':
			case 'A':
				this->num_buffer_ |= 10;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'b':
			case 'B':
				this->num_buffer_ |= 11;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'c':
			case 'C':
				this->num_buffer_ |= 12;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'd':
			case 'D':
				this->num_buffer_ |= 13;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'e':
			case 'E':
				this->num_buffer_ |= 14;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			case 'f':
			case 'F':
				this->num_buffer_ |= 15;
				this->state_ = ST_STRING_SYMBOL_4;
				break;

			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_STRING_SYMBOL_4:
			this->num_buffer_ <<= 4;
			switch (current_symbol) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->num_buffer_ |= current_symbol - '0';
				this->state_ = ST_STRING;
				break;

			case 'a':
			case 'A':
				this->num_buffer_ |= 10;
				this->state_ = ST_STRING;
				break;

			case 'b':
			case 'B':
				this->num_buffer_ |= 11;
				this->state_ = ST_STRING;
				break;

			case 'c':
			case 'C':
				this->num_buffer_ |= 12;
				this->state_ = ST_STRING;
				break;

			case 'd':
			case 'D':
				this->num_buffer_ |= 13;
				this->state_ = ST_STRING;
				break;

			case 'e':
			case 'E':
				this->num_buffer_ |= 14;
				this->state_ = ST_STRING;
				break;

			case 'f':
			case 'F':
				this->num_buffer_ |= 15;
				this->state_ = ST_STRING;
				break;

			default:
				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			CHECK_EXPECTED(utf8::add(this->buffer_, (wchar_t)this->num_buffer_));
			break;

		case ST_OBJECT_PROPERTY_NAME:
			switch (current_symbol) {
			case ':':
				this->state_ = ST_OBJECT_PROPERTY_VALUE;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		case ST_OBJECT_PROPERTY_VALUE:
			switch (current_symbol) {
			case '\"':
				this->state_ = ST_STRING;
				break;

			case '{':
				CHECK_EXPECTED(this->start_object());
				break;

			case '[':
				CHECK_EXPECTED(this->start_array());
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				this->state_ = ST_INTEGER;
				this->buffer_ += current_symbol;
				break;

			default:
				if (isspace(current_symbol)) {
					continue;
				}

				return vds::make_unexpected<parse_error>(
					this->stream_name_,
					this->line_,
					this->column_,
					std::string("Unexpected char ") + current_symbol);
			}
			break;

		default:
			return vds::make_unexpected<parse_error>(
				this->stream_name_,
				this->line_,
				this->column_,
				std::string("Unexpected char ") + current_symbol);
		}
	}

	return expected<void>();
}

vds::expected<void> vds::json_parser::final_data() {
	switch (this->state_) {
	case ST_EOF:
		break;

	case ST_BOF:
		if (this->parse_options_.enable_multi_root_objects) {
			break;
		}
		//break;

	default:
		return vds::make_unexpected<parse_error>(
			this->stream_name_,
			this->line_,
			this->column_,
			"Unexpected end of data");
	}
	return expected<void>();
}

vds::expected<void> vds::json_parser::start_object() {
	auto new_object = std::make_shared<json_object>(this->line_, this->column_);

	switch (this->state_) {
	case ST_OBJECT_PROPERTY_VALUE:
		std::static_pointer_cast<json_property>(this->current_object_)->value(new_object);
		break;
	case ST_BOF:
		this->saved_states_.push(ST_BOF);
		break;
	case ST_ARRAY:
		std::static_pointer_cast<json_array>(this->current_object_)->add(new_object);
		this->saved_states_.push(ST_ARRAY_ITEM);
		this->current_path_.push(this->current_object_);
		break;
	default:
		return vds::make_unexpected<parse_error>(
			this->stream_name_,
			this->line_,
			this->column_,
			"Unexpected state");
	}

	this->current_object_ = new_object;
	this->state_ = ST_OBJECT;
	return expected<void>();
}

vds::expected<void> vds::json_parser::final_object() {
	this->state_ = this->saved_states_.top();
	this->saved_states_.pop();

	if (ST_BOF == this->state_) {
		if (!this->parse_options_.enable_multi_root_objects) {
			this->state_ = ST_EOF;
		}

		return this->result_(this->root_object_);
	}
	else {
		this->current_object_ = this->current_path_.top();
		this->current_path_.pop();
		return expected<void>();
	}
}

void vds::json_parser::start_property() {
	auto new_property = std::make_shared<json_property>(this->line_, this->column_);
	new_property->name(this->buffer_);

	std::static_pointer_cast<json_object>(this->current_object_)->add_property(new_property);
	this->current_path_.push(this->current_object_);

	this->current_object_ = new_property;
	this->buffer_.clear();
}

void vds::json_parser::final_string_property() {
	std::static_pointer_cast<json_property>(this->current_object_)->value(
		std::make_shared<json_primitive>(this->line_, this->column_, this->buffer_)
	);

	this->current_object_ = this->current_path_.top();
	this->current_path_.pop();

	this->buffer_.clear();
}

void vds::json_parser::final_integer_property() {
	std::static_pointer_cast<json_property>(this->current_object_)->value(
		std::make_shared<json_primitive>(this->line_, this->column_, this->buffer_, json_primitive::primitive_type::integer)
	);

	this->current_object_ = this->current_path_.top();
	this->current_path_.pop();

	this->buffer_.clear();
}

vds::expected<void> vds::json_parser::after_slesh() {
	if (!this->parse_options_.enable_inline_comments) {
		return vds::make_unexpected<parse_error>(
			this->stream_name_,
			this->line_,
			this->column_,
			"Unexpected char /");
	}

	this->saved_states_.push(this->state_);
	this->state_ = ST_AFTER_SLESH;
	return expected<void>();
}

vds::async_task<vds::expected<void>> vds::json_parser::write_async(
	const unsigned char* data,
	size_t len) {
	co_return this->write(data, len);
}

vds::expected<std::shared_ptr<vds::json_value>> vds::json_parser::parse(const std::string &stream_name, const const_data_buffer & data) {
	std::shared_ptr<json_value> result;
	auto parser = std::make_shared<json_parser>(stream_name, [&result](const std::shared_ptr<json_value> & items) {
		result = items;
		return expected<void>();
	});
	CHECK_EXPECTED(parser->write(data.data(), data.size()));
	CHECK_EXPECTED(parser->write(nullptr, 0));

	return result;
}

vds::expected<std::shared_ptr<vds::json_value>> vds::json_parser::parse(const std::string &stream_name, expected<const_data_buffer> && data) {
	CHECK_EXPECTED_ERROR(data);

	return parse(stream_name, data.value());
}
