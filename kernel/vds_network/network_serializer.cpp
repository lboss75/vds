/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "network_serializer.h"

vds::network_serializer & vds::network_serializer::operator<<(uint8_t value)
{
  this->data_.push_back(value);
  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint16_t value)
{
  auto data = htons(value);

  this->data_.push_back((uint8_t)(0xFF & data));

  data >>= 8;
  this->data_.push_back((uint8_t)(0xFF & data));

  return *this;
}

vds::network_serializer & vds::network_serializer::operator<<(uint32_t value)
{
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
  (*this) << (uint16_t)value.length();
  for (size_t i = 0; i < value.length(); ++i) {
    (*this) << (uint8_t)value[i];
  }
  return *this;
}

void vds::network_serializer::start(uint16_t command_id)
{
  (*this) << (uint8_t)'V' << (uint8_t)'D' << (uint8_t)'S' << (uint8_t)'M';
  (*this) << (uint16_t)command_id;
  (*this) << (uint32_t)0;
}

void vds::network_serializer::final()
{
  auto data = htonl(this->data_.size());

  this->data_[6] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[7] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[8] = (uint8_t)(0xFF & data);

  data >>= 8;
  this->data_[9] = (uint8_t)(0xFF & data);
}
