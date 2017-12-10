#ifndef __VDS_CORE_BINARY_SERIALIZE_H_
#define __VDS_CORE_BINARY_SERIALIZE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "types.h"
#include "const_data_buffer.h"

namespace vds {

  class binary_serializer
  {
  public:
    //1 byte
    binary_serializer & operator << (uint8_t value);

    //2 byte
    binary_serializer & operator << (uint16_t value);

    //4 bytes
    binary_serializer & operator << (uint32_t value);

    //8 bytes
    binary_serializer & operator << (uint64_t value);

    binary_serializer & write_number(uint64_t value);

    binary_serializer & operator << (const std::string & value);
    
    binary_serializer & push_data(const void * data, size_t size, bool serialize_size = true);
    
    binary_serializer & operator << (const const_data_buffer & data);

    const std::vector<uint8_t> & data() const;
    
    uint8_t operator[](size_t index) const;
    uint8_t & operator[](size_t index);

    size_t size() const { return this->data_.size(); }
    
    template <typename T>
    binary_serializer & operator << (const std::list<T> & value)
    {
      this->write_number(value.size());
      for(auto & p : value){
        *this << p;
      }
      
      return *this;
    }
    
  private:
    std::vector<uint8_t> data_;
  };
  
  class binary_deserializer
  {
  public:
    binary_deserializer(const const_data_buffer & data);
    binary_deserializer(const_data_buffer && data) = delete;
    binary_deserializer(const void * data, size_t len);
    binary_deserializer(const std::vector<uint8_t> & data);
    binary_deserializer(std::vector<uint8_t> && data) = delete;
    
    //1 byte
    binary_deserializer & operator >> (uint8_t & value);

    //2 byte
    binary_deserializer & operator >> (uint16_t & value);

    //4 byte
    binary_deserializer & operator >> (uint32_t & value);

    //8 byte
    binary_deserializer & operator >> (uint64_t & value);

    binary_deserializer & operator >> (std::string & value);
   
    binary_deserializer & operator >> (const_data_buffer & data);
    
    uint64_t read_number();

    const uint8_t * data() const { return this->data_; }
    size_t size() const { return this->len_; }
    
    void pop_data(void * data, size_t & size, bool serialize_size);
    void pop_data(void * data, size_t size);

    template <typename T>
    binary_deserializer & operator >> (std::list<T> & value)
    {
      auto count = this->read_number();
      for(decltype(count) i = 0; i < count; ++i){
        T item;
        *this >> item;
        value.push_back(item);
      }
      
      return *this;
    }

  private:
    const uint8_t * data_;
    size_t len_;
  };
}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
