/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_serialize.h"

TEST(core_tests, test_serialize) {
  int size = std::rand();
  while (size < 1 || size > 1000) {
      size = std::rand();
  }
  
  std::list<serialize_step *> steps;
  for(int i = 0; i < size; ++i){
    switch(std::rand() % 7){
      case 0:
        steps.push_back(new primitive_serialize_step<uint8_t>());
        break;
      case 1:
        steps.push_back(new primitive_serialize_step<uint16_t>());
        break;
      case 2:
        steps.push_back(new primitive_serialize_step<uint32_t>());
        break;
      case 3:
        steps.push_back(new primitive_serialize_step<uint64_t>());
        break;
      case 4:
        steps.push_back(new test_write_number());
        break;
      case 5:
        steps.push_back(new test_write_string());
        break;
      case 6:
        steps.push_back(new test_write_buffer());
        break;
    }
  }

  
  vds::binary_serializer s;
  for(auto & p : steps) {
    p->serialize(s);
  }
  
  vds::binary_deserializer ds(s.data());
  for(auto & p : steps) {
    p->deserialize(ds);
  }
}