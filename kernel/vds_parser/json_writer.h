#ifndef __VDS_PARSER_JSON_WRITER_H_
#define __VDS_PARSER_JSON_WRITER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <stack>

namespace vds {
  class json_writer
  {
  public:
    json_writer();

    std::string str() const;

    expected<void> write_string_value(const std::string & value);
    expected<void> write_null_value();

    expected<void> start_property(const std::string & name);
    expected<void> end_property();

    expected<void> write_property(const std::string & name, const std::string & value);

    expected<void> start_object();
    expected<void> end_object();

    expected<void> start_array();
    expected<void> end_array();

  private:
    std::stringstream stream_;

    enum State
    {
      BOF,
      START_OBJECT,
      OBJECT_BODY,
      PROPERTY,
      PROPERTY_END,
      START_ARRAY,
      ARRAY_BODY
    };

    State state_;
    std::stack<State> state_path_;

    expected<void> write_string(const std::string & value);
  };
}

#endif // __VDS_PARSER_JSON_WRITER_H_
