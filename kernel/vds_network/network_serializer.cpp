/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "network_serializer.h"

vds::network_serializer & vds::network_serializer::operator<<(uint8_t value)
{
  this->data_.push_back(1);
  this->data_.push_back(value);
  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint16_t value)
{
  this->data_.push_back(2);
  auto data = htons(value);

  this->data_.push_back((uint8_t)(0xFF & data));

  data >>= 8;
  this->data_.push_back((uint8_t)(0xFF & data));

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint32_t value)
{
  this->data_.push_back(4);
  auto data = htonl(value);

  this->data_.push_back((uint8_t)(0xFF & data));

  data >>= 8;
  this->data_.push_back((uint8_t)(0xFF & data));

  data >>= 8;
  this->data_.push_back((uint8_t)(0xFF & data));

  data >>= 8;
  this->data_.push_back((uint8_t)(0xFF & data));

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(const std::string & value)
{
  this->data_.push_back(3);
  (*this) << (uint16_t)value.length();
  for (size_t i = 0; i < value.length(); ++i) {
    this->data_.push_back((uint8_t)value[i]);
  }
  return *this;
}

void vds::network_serializer::start(uint8_t command_id)
{
  this->data_.push_back((uint8_t)'V');
  this->data_.push_back((uint8_t)'D');
  this->data_.push_back((uint8_t)'S');
  this->data_.push_back((uint8_t)'M');
  
  this->data_.push_back(command_id);
  
  this->data_.push_back((uint8_t)0);
  this->data_.push_back((uint8_t)0);
  this->data_.push_back((uint8_t)0);
  this->data_.push_back((uint8_t)0);
}

void vds::network_serializer::final()
{
  this->data_.push_back(0);

  auto data = htonl((u_long)this->data_.size());

  this->data_[5] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[6] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[7] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[8] = (uint8_t)(0xFF & data);
}

const std::vector<uint8_t>& vds::network_serializer::data() const
{
  return this->data_;
}

vds::network_serializer& vds::network_serializer::push_data(const void* data, size_t len)
{
  this->data_.push_back(5);
  *this << (uint32_t)len;
  this->data_.insert(this->data_.end(), (const uint8_t *)data, (const uint8_t *)data + len);
  
  return *this;
}

vds::network_serializer& vds::network_serializer::operator<<(const data_buffer & data)
{
  this->push_data(data.data(), data.size());
  return *this;
}
/////////////////////////////////////////////////////////
vds::network_deserializer::network_deserializer(const void * data, size_t len)
: data_((const uint8_t *)data), len_(len)
{
}

uint8_t vds::network_deserializer::start()
{
  if(8 > this->len_
    || (uint8_t)'V' != this->data_[0]
    || (uint8_t)'D' != this->data_[1]
    || (uint8_t)'S' != this->data_[2]
    || (uint8_t)'M' != this->data_[3]){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  
  uint16_t result = this->data_[4];
  
  uint32_t len = this->data_[5];
  len = (len << 8) | this->data_[6];
  len = (len << 8) | this->data_[7];
  len = (len << 8) | this->data_[8];
  
  if(ntohl(len) != this->len_){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  this->data_ += 8;
  this->len_ -= 8;
  
  return result;
}

void vds::network_deserializer::final()
{
  if(1 != this->len_
    || 0 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  this->len_--;
  this->data_++;
}

vds::network_deserializer& vds::network_deserializer::operator>> (uint8_t& value)
{
  if(2 > this->len_
    || 1 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  value = this->data_[1];
  
  this->len_ -= 2;
  this->data_ += 2;
  
  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>> (uint16_t& value)
{
  if(3 > this->len_
    || 2 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  value = ntohs(this->data_[1] | (this->data_[2] << 8));
  
  this->len_ -= 3;
  this->data_ += 3;
  
  return *this;
}


vds::network_deserializer& vds::network_deserializer::operator>>(uint32_t& value)
{
  if(5 > this->len_
    || 4 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  value = ntohl(this->data_[1] | (this->data_[2] << 8) | (this->data_[3] << 16) | (this->data_[4] << 24));
  
  this->len_ -= 5;
  this->data_ += 5;
  
  return *this;
}


vds::network_deserializer& vds::network_deserializer::operator>>(std::string& value)
{
  if(5 > this->len_
    || 3 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  this->len_--;
  this->data_++;

  uint16_t len;
  *this >> len;
  
  if(len > this->len_){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  value.reserve(len);
  for(uint16_t i = 0; i < len; ++i){
    value += (char)this->data_[i];
  }
  
  this->len_ -= len;
  this->data_ += len;
  
  return *this;
}

vds::network_deserializer& vds::network_deserializer::read_data(std::vector< uint8_t >& data)
{
  if(5 > this->len_
    || 5 != this->data_[0]){
    throw new std::runtime_error("Invalid binary message format");
  }
  this->len_--;
  this->data_++;

  uint32_t len;
  *this >> len;
  
  if(len > this->len_){
    throw new std::runtime_error("Invalid binary message format");
  }
  
  data.reserve(len);
  for(uint16_t i = 0; i < len; ++i){
    data.push_back(this->data_[i]);
  }
  
  this->len_ -= len;
  this->data_ += len;
  
  return *this;
}

vds::network_deserializer& vds::network_deserializer::operator>>(data_buffer& data)
{
  std::vector<uint8_t> result;
  this->read_data(result);
  
  data.reset(result.data(), result.size());
  return *this;
}
