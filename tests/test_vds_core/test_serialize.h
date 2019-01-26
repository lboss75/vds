#ifndef TEST_SERIALIZE_H
#define TEST_SERIALIZE_H

#include "binary_serialize.h"

class serialize_step
{
public:
  serialize_step()
  {
  }
  
  virtual ~serialize_step()
  {
  }
  
  virtual vds::expected<void> serialize(vds::binary_serializer& s) = 0;
  virtual vds::expected<void> deserialize(vds::binary_deserializer& s) = 0;
};

template <typename data_type>
class primitive_serialize_step : public serialize_step
{
public:
  primitive_serialize_step()
  : data_((data_type)std::rand())
  {    
  }
  
  ~primitive_serialize_step()
  {
  }
  
  virtual vds::expected<void> serialize(vds::binary_serializer& s)
  {
    return vds::serialize(s, this->data_);
  }

  virtual vds::expected<void> deserialize(vds::binary_deserializer& s)
  {
    data_type data;
    CHECK_EXPECTED(vds::deserialize(s, data));

    if (data != this->data_) {
      return vds::make_unexpected<std::runtime_error>("Invalid deserialize");
    }
    else {
      return vds::expected<void>();
    }
  }

private:
  data_type data_;
};

class test_write_number : public serialize_step
{
public:
  test_write_number()
  : data_((uint64_t)std::rand())
  {    
  }
  
  ~test_write_number()
  {
  }
  
  virtual vds::expected<void> serialize(vds::binary_serializer& s)
  {
    return s.write_number(this->data_);
  }
  
  virtual vds::expected<void> deserialize(vds::binary_deserializer& s)
  {
    GET_EXPECTED(data, s.read_number());
    if(data != this->data_) {
      return vds::make_unexpected<std::runtime_error>("Invalid deserialize");
    }
    else {
      return vds::expected<void>();
    }
  }
  
private:
  uint64_t data_;
};

class test_write_string : public serialize_step
{
public:
  test_write_string()
  {    
    int size = std::rand();
    while (size > 1000) {
        size = std::rand();
    }
    
    for(int i = 0; i < size; ++i){
      this->data_ = (char)std::rand();
    }
  }
  
  ~test_write_string()
  {
  }
  
  virtual vds::expected<void> serialize(vds::binary_serializer& s)
  {
    return vds::serialize(s, this->data_);
  }
  
  virtual vds::expected<void> deserialize(vds::binary_deserializer& s)
  {
    std::string data;
    CHECK_EXPECTED(vds::deserialize(s, data));
    
    if (data != this->data_) {
      return vds::make_unexpected<std::runtime_error>("Invalid deserialize");
    }
    else {
      return vds::expected<void>();
    }
  }
  
private:
  std::string data_;
};

class test_write_buffer : public serialize_step
{
public:
  test_write_buffer()
  {    
    int size = std::rand();
    while (size > 1000) {
        size = std::rand();
    }
    
    this->data_.resize(size);
    for(int i = 0; i < size; ++i){
      this->data_[i] = (uint8_t)std::rand();
    }
  }
  
  ~test_write_buffer()
  {
  }
  
  virtual vds::expected<void> serialize(vds::binary_serializer& s)
  {
    return vds::serialize(s, this->data_);
  }

  virtual vds::expected<void> deserialize(vds::binary_deserializer& s)
  {
    vds::const_data_buffer data;
    CHECK_EXPECTED(vds::deserialize(s, data));

    if (data != this->data_) {
      return vds::make_unexpected<std::runtime_error>("Invalid deserialize");
    }
    else {
      return vds::expected<void>();
    }
  }

private:
  vds::const_data_buffer data_;
};


#endif // TEST_SERIALIZE_H
