/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "binary_serialize.h"

vds::binary_serializer& vds::binary_serializer::operator << (uint8_t value)
{
  this->data_.push_back(value);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint32_t value)
{
  this->data_.push_back((value >> 24) & 0xFF);
  this->data_.push_back((value >> 16) & 0xFF);
  this->data_.push_back((value >> 8) & 0xFF);
  this->data_.push_back(value & 0xFF);
  return *this;
}

vds::binary_serializer& vds::binary_serializer::operator << (uint64_t value)
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

template <unsigned int bit_count>
class number_serializer;

class number_serializer<0>
{
public:
  constexpr uint64_t max_value = 128;

  static vds::binary_serializer & write_number(vds::binary_serializer & s, uint64_t value)
  {
    assert(max_value > value);
    return s << (uint8_t)value;
  }
};

template <unsigned int bit_count>
class number_serializer<bit_count>
{
public:
  // 11..1 (bit count), 0, xxx (bit count - 1)
  constexpr uint64_t max_value = number_serializer<bit_count - 1>::max_value + (1u64 << (6 * ((1u64 << bit_count) - 1) + (1u64 << (7 - 2 * bit_count))));

  static vds::binary_serializer & write_number(vds::binary_serializer & s, uint64_t value)
  {

    return s << (uint8_t)value;
  }
};

vds::binary_serializer & vds::binary_serializer::write_number(uint64_t value)
{
  // 0 .... 7 bit
  if (128 > value) {
    return *this << (uint8_t)value;
  }

  // 1100 .... 4 bit, 10 ... 6 bit = 10 bit, 128 + 10 bit
  if (0x100 > value) {
    return *this 
    << (0b11000000 | ((uint8_t)(value >> 6) & 0x3F))
    << (0b10000000 | (uint8_t)(value & 0x3F));

  }

  // 1101 .... 4 bit, 10 ... 6 bit, 10 ... 6 bit = 16 bit
  // 1110 00.. 2 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 20 bit
  // 1110 01.. 2 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 26 bit
  // 1110 10.. 2 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 32 bit
  // 1110 11.. 2 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 38 bit
  // 1111 0000 0 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 42 bit
  // 1111 0001 0 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit, 10 ... 6 bit = 48 bit

}

vds::binary_serializer& vds::binary_serializer::operator<<(const std::string & value)
{
}

vds::binary_serializer & vds::binary_serializer::push_data(const void * data, size_t size, bool serialize_size)
{
  if (serialize_size) {
    *this << size;
  }
  while (0 < size) {

  }
}


vds::binary_serializer& vds::binary_serializer::operator << (const data_buffer& data)
{

}

const std::vector< uint8_t >& vds::binary_serializer::data() const
{
  return this->data_;
}
