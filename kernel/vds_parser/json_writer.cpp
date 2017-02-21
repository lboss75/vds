/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "json_writer.h"

vds::json_writer::json_writer()
  : state_(BOF)
{
}

std::string vds::json_writer::str() const
{
  return this->stream_.str();
}


void vds::json_writer::write_string_value(const std::string & value)
{
  switch (this->state_)
  {
  case PROPERTY:
    this->state_ = PROPERTY_END;
    break;

  case ARRAY_BODY:
    this->stream_ << ',';
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->write_string(value);
}

void vds::json_writer::write_null_value()
{
  switch (this->state_)
  {
  case PROPERTY:
    this->state_ = PROPERTY_END;
    break;

  case ARRAY_BODY:
    this->stream_ << ',';
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->stream_ << "null";
}

void vds::json_writer::start_property(const std::string & name)
{
  switch (this->state_)
  {
  case START_OBJECT:
    this->state_ = OBJECT_BODY;
    break;
  case OBJECT_BODY:
    this->stream_ << ',';
    break;
  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_path_.push(this->state_);
  this->state_ = PROPERTY;

  this->write_string(name);
  this->stream_ << ':';
}

void vds::json_writer::end_property()
{
  switch (this->state_)
  {
  case PROPERTY_END:
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_ = this->state_path_.top();
  this->state_path_.pop();
}

void vds::json_writer::write_property(const std::string & name, const std::string & value)
{
  this->start_property(name);
  this->write_string_value(value);
  this->end_property();
}

void vds::json_writer::start_object()
{
  switch (this->state_)
  {
  case BOF:
  //case START_OBJECT:
  //case OBJECT_BODY:
  case START_ARRAY:
  case ARRAY_BODY:
    this->state_path_.push(ARRAY_BODY);
    break;
      
  case PROPERTY:
    this->state_path_.push(PROPERTY_END);
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_ = START_OBJECT;

  this->stream_ << '{';
}

void vds::json_writer::end_object()
{
  switch (this->state_)
  {
  case START_OBJECT:
  case OBJECT_BODY:
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_ = this->state_path_.top();
  this->state_path_.pop();

  this->stream_ << '}';
}

void vds::json_writer::start_array()
{
  switch (this->state_)
  {
  case BOF:
  case START_OBJECT:
  case OBJECT_BODY:
  case PROPERTY:
  case START_ARRAY:
  case ARRAY_BODY:
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_path_.push(this->state_);
  this->state_ = START_ARRAY;

  this->stream_ << '[';
}

void vds::json_writer::end_array()
{
  switch (this->state_)
  {
  case START_ARRAY:
  case ARRAY_BODY:
    break;

  default:
    throw new std::runtime_error("Invalid json_writer state");
  }

  this->state_ = this->state_path_.top();
  this->state_path_.pop();

  this->stream_ << ']';
}

void vds::json_writer::write_string(const std::string & value)
{
  this->stream_ << '\"';
  const char * utf8string = value.c_str();
  size_t len = value.length();

  while(0 < len) {
    auto ch = utf8::next_char(utf8string, len);
    switch (ch) {
    case '\\':
      this->stream_ << "\\\\";
      break;
    case '\"':
      this->stream_ << "\\\"";
      break;
    case '\n':
      this->stream_ << "\\n";
      break;
    case '\r':
      this->stream_ << "\\r";
      break;
    case '\b':
      this->stream_ << "\\b";
      break;
    case '\t':
      this->stream_ << "\\t";
      break;
    case '\f':
      this->stream_ << "\\f";
      break;
    default:
      if (ch < 0x80 && isprint((int)ch)) {
        this->stream_ << (char)ch;
      }
      else {
        this->stream_ << "\\x" << std::setw(4) << std::setfill('0') << std::hex << (uint16_t)ch;
      }
    }
  }

  this->stream_ << '\"';
}
