#ifndef __VDS_NETWORK_NETWORK_SERIALIZER_H_
#define __VDS_NETWORK_NETWORK_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class network_serializer
  {
  public:
    //1 byte
    network_serializer & operator << (uint8_t value);

    //2 byte
    network_serializer & operator << (uint16_t value);

    //4 byte
    network_serializer & operator << (uint32_t value);

    network_serializer & operator << (const std::string & value);
    
    network_serializer & push_data(const void * data, size_t len);
    
    network_serializer & operator << (const data_buffer & data);

    void start(uint8_t command_id);
    void final();

    const std::vector<uint8_t> & data() const;

  private:
    std::vector<uint8_t> data_;
  };
  
  class network_deserializer
  {
  public:
    network_deserializer(const void * data, size_t len);
    
    //1 byte
    network_deserializer & operator >> (uint8_t & value);

    //2 byte
    network_deserializer & operator >> (uint16_t & value);

    //4 byte
    network_deserializer & operator >> (uint32_t & value);

    network_deserializer & operator >> (std::string & value);
    
    network_deserializer & operator >> (data_buffer & data);

    network_deserializer & read_data(std::vector<uint8_t> & data);

    uint8_t start();
    void final();

  private:
    const uint8_t * data_;
    size_t len_;
  };
}

#endif // __VDS_NETWORK_NETWORK_SERIALIZER_H_
