#ifndef __VDS_PARSER_JSON_WRITER_H_
#define __VDS_PARSER_JSON_WRITER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class json_writer
  {
  public:
    json_writer();

    std::string str() const;

    void write_string_value(const std::string & value);
    void write_null_value();

    void start_property(const std::string & name);
    void end_property();

    void write_property(const std::string & name, const std::string & value);

    void start_object();
    void end_object();

    void start_array();
    void end_array();

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

    void write_string(const std::string & value);
  };
}

#endif // __VDS_PARSER_JSON_WRITER_H_
