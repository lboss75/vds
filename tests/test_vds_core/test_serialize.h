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
  
  virtual void serialize(vds::binary_serializer& s) = 0;
  virtual void deserialize(vds::binary_deserializer& s) = 0;
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
  
  virtual void serialize(vds::binary_serializer& s)
  {
    s << this->data_;
  }
  
  virtual void deserialize(vds::binary_deserializer& s)
  {
    data_type data;
    s >> data;
    
    ASSERT_EQ(data, this->data_);    
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
  
  virtual void serialize(vds::binary_serializer& s)
  {
    s.write_number(this->data_);
  }
  
  virtual void deserialize(vds::binary_deserializer& s)
  {
    uint64_t data = s.read_number();
    
    ASSERT_EQ(data, this->data_);    
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
  
  virtual void serialize(vds::binary_serializer& s)
  {
    s << this->data_;
  }
  
  virtual void deserialize(vds::binary_deserializer& s)
  {
    std::string data;
    s >> data;
    
    ASSERT_EQ(data, this->data_);    
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
    
    std::vector<uint8_t> buffer;
    for(int i = 0; i < size; ++i){
      buffer.push_back((uint8_t)std::rand());
    }
    
    this->data_.reset(buffer.data(), buffer.size());
  }
  
  ~test_write_buffer()
  {
  }
  
  virtual void serialize(vds::binary_serializer& s)
  {
    s << this->data_;
  }
  
  virtual void deserialize(vds::binary_deserializer& s)
  {
    vds::const_data_buffer data;
    s >> data;
    
    ASSERT_EQ(data, this->data_);
  }
  
private:
  vds::const_data_buffer data_;
};


#endif // TEST_SERIALIZE_H
