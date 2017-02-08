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

    void start(uint16_t command_id);
    void final();

  private:
    std::vector<uint8_t> data_;
  };
}

#endif // __VDS_NETWORK_NETWORK_SERIALIZER_H_
