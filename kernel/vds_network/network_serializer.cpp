/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "network_serializer.h"

vds::network_serializer & vds::network_serializer::operator<<(uint8_t value)
{
  this->data_ << (uint8_t)1 << value;
  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint16_t value)
{
  this->data_ << (uint8_t)2 << value;

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint32_t value)
{
  this->data_ << (uint8_t)4 << value;

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint64_t value)
{
  this->data_ << (uint8_t)8 << value;

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(const std::string & value)
{
  this->data_ << (uint8_t)3 << value;
  
  return *this;
}

void vds::network_serializer::start(uint8_t command_id)
{
  this->data_ << (uint8_t)'V' << (uint8_t)'D' << (uint8_t)'S' << (uint8_t)'M';
  this->data_ << command_id;
  
  this->data_ << (uint16_t)0;
}

void vds::network_serializer::final()
{
  this->data_ << (uint8_t)0;
  
  if (0xFFFF < this->data_.size()) {
    throw std::runtime_error("Data too long for datagram communication");
  }

  uint16_t data = (uint16_t)this->data_.size();

  this->data_[6] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[5] = (uint8_t)(0xFF & data);
}

const std::vector<uint8_t>& vds::network_serializer::data() const
{
  return this->data_.data();
}

vds::network_serializer& vds::network_serializer::push_data(const void* data, size_t len)
{
  this->data_ << (uint8_t)5;
  this->data_.push_data(data, len, true);
  
  return *this;
}

vds::network_serializer& vds::network_serializer::operator<<(const const_data_buffer & data)
{
  this->push_data(data.data(), data.size());
  return *this;
}
/////////////////////////////////////////////////////////
vds::network_deserializer::network_deserializer(const void * data, size_t len)
: data_(data, len)
{
}

uint8_t vds::network_deserializer::start()
{
  uint8_t magic1;
  uint8_t magic2;
  uint8_t magic3;
  uint8_t magic4;
  
  this->data_ >> magic1 >> magic2 >> magic3 >> magic4;
  
  if((uint8_t)'V' != magic1
    || (uint8_t)'D' != magic2
    || (uint8_t)'S' != magic3
    || (uint8_t)'M' != magic4){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t result;
  this->data_ >> result;
  
  uint16_t len;
  this->data_ >> len;
  
  if(len != this->data_.size() + 7){
    throw std::runtime_error("Invalid binary message format");
  }
 
  return result;
}

void vds::network_deserializer::final()
{
  if(1 != this->data_.size()){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t ct;
  this->data_ >> ct;
  
  if(0 != ct){
    throw std::runtime_error("Invalid binary message format");
  }
}

vds::network_deserializer& vds::network_deserializer::operator>> (uint8_t& value)
{
  if(2 > this->data_.size()){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t ct;
  this->data_ >> ct;
  
  if(1 != ct){
    throw std::runtime_error("Invalid binary message format");
  }
  
  this->data_ >> value;
  
  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>> (uint16_t& value)
{
  if(3 > this->data_.size()){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t ct;
  this->data_ >> ct;
  
  if(2 != ct){
    throw std::runtime_error("Invalid binary message format");
  }
  
  this->data_ >> value;
  
  return *this;
}


vds::network_deserializer& vds::network_deserializer::operator>>(uint32_t& value)
{
  if(5 > this->data_.size()){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t ct;
  this->data_ >> ct;
  
  if(4 != ct){
    throw std::runtime_error("Invalid binary message format");
  }
  
  this->data_ >> value;

  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>>(uint64_t& value)
{
  if (9 > this->data_.size()) {
    throw std::runtime_error("Invalid binary message format");
  }

  uint8_t ct;
  this->data_ >> ct;

  if (8 != ct) {
    throw std::runtime_error("Invalid binary message format");
  }

  this->data_ >> value;
  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>>(std::string& value)
{
  if(5 > this->data_.size()){
    throw std::runtime_error("Invalid binary message format");
  }
  
  uint8_t ct;
  this->data_ >> ct;
  
  if(3 != ct){
    throw std::runtime_error("Invalid binary message format");
  }
  
  this->data_ >> value;

  return *this;
}

vds::network_deserializer& vds::network_deserializer::read_data(std::vector< uint8_t >& data)
{
  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>>(const_data_buffer& data)
{
  if (1 > this->data_.size()) {
    throw std::runtime_error("Invalid binary message format");
  }

  uint8_t ct;
  this->data_ >> ct;

  if (5 != ct) {
    throw std::runtime_error("Invalid binary message format");
  }

  this->data_ >> data;
  
  return *this;
}
