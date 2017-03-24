#ifndef __VDS_CORE_BINARY_SERIALIZE_H_
#define __VDS_CORE_BINARY_SERIALIZE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"

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
    
    binary_serializer & operator << (const data_buffer & data);

    const std::vector<uint8_t> & data() const;
    
    uint8_t operator[](size_t index) const;
    uint8_t & operator[](size_t index);

    size_t size() const { return this->data_.size(); }
    
  private:
    std::vector<uint8_t> data_;
  };
  
  class binary_deserializer
  {
  public:
    binary_deserializer(const data_buffer & data);
    binary_deserializer(const void * data, size_t len);
    binary_deserializer(const std::vector<uint8_t> & data);
    
    //1 byte
    binary_deserializer & operator >> (uint8_t & value);

    //2 byte
    binary_deserializer & operator >> (uint16_t & value);

    //4 byte
    binary_deserializer & operator >> (uint32_t & value);

    //8 byte
    binary_deserializer & operator >> (uint64_t & value);

    binary_deserializer & operator >> (std::string & value);
   
    binary_deserializer & operator >> (data_buffer & data);
    
    uint64_t read_number();
    
    size_t size() const { return this->len_; }

  private:
    const uint8_t * data_;
    size_t len_;
  };
}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
