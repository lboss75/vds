#ifndef __VDS_PARSER_JSON_PARSER_H_
#define __VDS_PARSER_JSON_PARSER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <stack>
#include "json_object.h"
#include "parse_error.h"
#include "encoding.h"
#include "stream.h"

namespace vds {
  class json_parser : public stream_output_async<uint8_t> {
  public:
    struct options {
      bool enable_inline_comments;
      bool enable_multi_root_objects;

      options()
        : enable_inline_comments(false),
        enable_multi_root_objects(false) {
      }
    };

    json_parser(
      const std::string &stream_name,
      const std::function<expected<void>(const std::shared_ptr<json_value> &)> &result,
      const options &parse_options = options());

	vds::async_task<vds::expected<void>> write_async(
		const unsigned char* data,
		size_t len) override;
    
    expected<void> write(
      const uint8_t * input_buffer,
      size_t input_len);

	static expected<std::shared_ptr<json_value>> parse(
		const std::string &stream_name,
		const const_data_buffer & data);

	static expected<std::shared_ptr<json_value>> parse(
		const std::string &stream_name,
		expected<const_data_buffer> && data);

  private:
    std::string stream_name_;
    std::function<expected<void>(const std::shared_ptr<json_value> &)> result_;
    options parse_options_;

    enum State {
      ST_BOF,

      ST_AFTER_SLESH,
      ST_INLINE_COMMENT,

      ST_ARRAY,
      ST_ARRAY_ITEM,

      ST_OBJECT,
      ST_OBJECT_ITEM,

      ST_NUMBER,

      ST_OBJECT_PROPERTY_NAME,
      ST_OBJECT_PROPERTY_VALUE,
      //ST_OBJECT_PROPERTY_VALUE_FINISH,

	  ST_INTEGER,

      ST_STRING,
      ST_STRING_BACKSLESH,

      ST_STRING_SYMBOL_1,
      ST_STRING_SYMBOL_2,
      ST_STRING_SYMBOL_3,
      ST_STRING_SYMBOL_4,

      ST_EOF
    };

    State state_;
    std::stack<State> saved_states_;

    std::shared_ptr<json_value> root_object_;
    std::stack<std::shared_ptr<json_value>> current_path_;
    std::shared_ptr<json_value> current_object_;

    int line_;
    int column_;

    std::string buffer_;
    uint32_t num_buffer_;

    expected<void> final_data();
	expected<void> after_slesh();
	expected<void> start_array();
	expected<void> final_array();
	expected<void> start_object();
	expected<void> final_object();

	void start_property();
	void final_string_property();
	void final_integer_property();
  };
  


  inline expected<void> operator >> (
    vds::binary_deserializer & s,
    std::shared_ptr<json_value> & value) {

    std::string value_str;
    CHECK_EXPECTED(s  >> value_str);

    GET_EXPECTED_VALUE(value, json_parser::parse(
      "deserialize",
      const_data_buffer(value_str.c_str(), value_str.length())));

    return expected<void>();
  }
}
#endif // __VDS_PARSER_JSON_PARSER_H_
