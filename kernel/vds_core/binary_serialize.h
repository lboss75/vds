#ifndef __VDS_CORE_BINARY_SERIALIZE_H_
#define __VDS_CORE_BINARY_SERIALIZE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  class binary_serializer
  {
  public:
    //1 byte
    binary_serializer & operator << (uint8_t value);

    //2 byte
    binary_serializer & operator << (uint16_t value);

    //4 byte
    binary_serializer & operator << (uint32_t value);

    binary_serializer & operator << (const std::string & value);
    
    binary_serializer & push_data(const void * data, size_t len);
    
    binary_serializer & operator << (const data_buffer & data);

    const std::vector<uint8_t> & data() const;

  private:
    std::vector<uint8_t> data_;
  };
  
  class binary_deserializer
  {
  public:
    binary_deserializer(const void * data, size_t len);
    
    //1 byte
    binary_deserializer & operator >> (uint8_t value);

    //2 byte
    binary_deserializer & operator >> (uint16_t value);

    //4 byte
    binary_deserializer & operator >> (uint32_t value);

    binary_deserializer & operator >> (const std::string & value);
   
    binary_deserializer & operator >> (const data_buffer & data);

  private:
    std::vector<uint8_t> data_;
  };
}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
