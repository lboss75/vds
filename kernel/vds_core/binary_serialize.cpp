/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "binary_serialize.h"

vds::binary_serializer& vds::binary_serializer::operator << (bool value)
{
  this->data_.add(value ? (uint8_t)0xFF : (uint8_t)0);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint8_t value)
{
  this->data_.add(value);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint16_t value)
{
  this->data_.add((value >> 8) & 0xFF);
  this->data_.add(value & 0xFF);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint32_t value)
{
  this->data_.add((value >> 24) & 0xFF);
  this->data_.add((value >> 16) & 0xFF);
  this->data_.add((value >> 8) & 0xFF);
  this->data_.add(value & 0xFF);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint64_t value)
{
  this->data_.add((value >> 56) & 0xFF);
  this->data_.add((value >> 48) & 0xFF);
  this->data_.add((value >> 40) & 0xFF);
  this->data_.add((value >> 32) & 0xFF);
  this->data_.add((value >> 24) & 0xFF);
  this->data_.add((value >> 16) & 0xFF);
  this->data_.add((value >> 8) & 0xFF);
  this->data_.add(value & 0xFF);
  return *this;
}

vds::binary_serializer & vds::binary_serializer::write_number(uint64_t value)
{
  // 0 .... 7 bit
  if (128 > value) {
    return *this << (uint8_t)value;
  }
  
  value -= 128;
  
  std::vector<uint8_t> data;
  do {
    data.push_back(value & 0xFF);
    value >>= 8;
  } while (0 != value);
  
  this->data_.add((uint8_t)(0x80 | data.size()));
  for(auto p = data.rbegin(); data.rend() != p; ++p){
    this->data_.add(*p);
  }
  
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator<<(const std::string & value)
{
  this->write_number(value.length());
  this->data_.add(value.c_str(), value.length());
  
  return *this;
}

vds::binary_serializer & vds::binary_serializer::push_data(const void * data, size_t size, bool serialize_size)
{
  if (serialize_size) {
    this->write_number(size);
  }
  
  this->data_.add(data, size);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (const const_data_buffer& data)
{
  return this->push_data(data.data(), data.size(), true);
}

///////////////////////////////////////////////////////////////////////////
vds::binary_deserializer::binary_deserializer(const const_data_buffer & data)
  : data_(data.data()), len_(data.size())
{
}

vds::binary_deserializer::binary_deserializer(const void* data, size_t len)
: data_((const uint8_t *)data), len_(len)
{
}

vds::binary_deserializer::binary_deserializer(const std::vector<uint8_t> & data)
: data_(data.data()), len_(data.size())
{
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(bool& value) {
  if (1 > this->len_) {
    throw std::runtime_error("Invalid data");
  }

  value = (0 != *this->data_++);
  --this->len_;

  return *this;
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(uint8_t & value)
{
  if(1 > this->len_){
    throw std::runtime_error("Invalid data");
  }
  
  value = *this->data_++;
  --this->len_;
  
  return *this;
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(uint16_t& value)
{
  if(2 > this->len_){
    throw std::runtime_error("Invalid data");
  }
  
  value = *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  
  this->len_ -= 2;
  
  return *this;
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(uint32_t& value)
{
  if(4 > this->len_){
    throw std::runtime_error("Invalid data");
  }
  
  value = *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  
  this->len_ -= 4;
  
  return *this;
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(uint64_t& value)
{
  if(8 > this->len_){
    throw std::runtime_error("Invalid data");
  }
  
  value = *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  value <<= 8;
  value |= *this->data_++;
  
  this->len_ -= 8;
  
  return *this;
}

vds::binary_deserializer& vds::binary_deserializer::operator>>(std::string& value)
{
  auto len = this->read_number();
  value.resize(len);
  for(uint64_t i = 0; i < len; ++i){
    uint8_t ch;
    *this >> ch;
    value[i] = ch;
  }
  
  return *this;
}


vds::binary_deserializer& vds::binary_deserializer::operator>>(vds::const_data_buffer& data)
{
  auto len = this->read_number();
  if(len > 1024 * 1024 * 1024){
    throw std::runtime_error("very big object");
  }
  std::vector<uint8_t> buffer(len);
  for(uint64_t i = 0; i < len; ++i){
    uint8_t ch;
    *this >> ch;
    buffer[i] = ch;
  }
  
  data.reset(buffer.data(), len);

  return *this;
}

void vds::binary_deserializer::pop_data(void* data, size_t& size, bool serialize_size)
{
  if(serialize_size){
    auto len = this->read_number();
    if(size < len){
      throw std::runtime_error("Buffer too small");
    }
    size = len;
  }
  uint8_t * p = (uint8_t *)data;
  for(uint64_t i = 0; i < size; ++i){
    uint8_t ch;
    *this >> ch;
    *p++ = ch;
  }
}

size_t vds::binary_deserializer::pop_data(void* data, size_t size)
{
  this->pop_data(data, size, true);
  return size;
}


uint64_t vds::binary_deserializer::read_number()
{
  uint8_t value;
  *this >> value;
  
  if(0x80 > value){
    return value;
  }
  
  uint64_t result = 0;
  for(uint8_t i = (value & 0x7F); i > 0; --i){
    *this >> value;
    result <<= 8;
    result |= value;
  }
  
  return result + 0x80;
}
