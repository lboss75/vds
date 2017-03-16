/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "binary_serialize.h"

vds::binary_serializer& vds::binary_serializer::operator<<(uint8_t value)
{
  this->data_.push_back(value);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator<<(uint32_t value)
{
  this->data_.push_back((value >> 24) & 0xFF);
  this->data_.push_back((value >> 16) & 0xFF);
  this->data_.push_back((value >> 8) & 0xFF);
  this->data_.push_back(value & 0xFF);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator<<(uint64_t value)
{
  this->data_.push_back((value >> 48) & 0xFF);
  this->data_.push_back((value >> 40) & 0xFF);
  this->data_.push_back((value >> 32) & 0xFF);
  this->data_.push_back((value >> 24) & 0xFF);
  this->data_.push_back((value >> 16) & 0xFF);
  this->data_.push_back((value >> 8) & 0xFF);
  this->data_.push_back(value & 0xFF);
  return *this;

}

vds::binary_serializer& vds::binary_serializer::operator<<(const std::string& value)
{

}


vds::binary_serializer& vds::binary_serializer::operator<<(const data_buffer& data)
{

}

const std::vector< uint8_t >& vds::binary_serializer::data() const
{
  return this->data_;
}
