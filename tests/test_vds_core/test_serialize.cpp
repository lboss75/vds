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
  
  std::list<std::unique_ptr<serialize_step>> steps;
  for(int i = 0; i < size; ++i){
    switch(std::rand() % 7){
      case 0:
        steps.push_back(std::unique_ptr<serialize_step>(new primitive_serialize_step<uint8_t>()));
        break;
      case 1:
        steps.push_back(std::unique_ptr<serialize_step>(new primitive_serialize_step<uint16_t>()));
        break;
      case 2:
        steps.push_back(std::unique_ptr<serialize_step>(new primitive_serialize_step<uint32_t>()));
        break;
      case 3:
        steps.push_back(std::unique_ptr<serialize_step>(new primitive_serialize_step<uint64_t>()));
        break;
      case 4:
        steps.push_back(std::unique_ptr<serialize_step>(new test_write_number()));
        break;
      case 5:
        steps.push_back(std::unique_ptr<serialize_step>(new test_write_string()));
        break;
      case 6:
        steps.push_back(std::unique_ptr<serialize_step>(new test_write_buffer()));
        break;
    }
  }

  
  vds::binary_serializer s;
  for(auto & p : steps) {
    p->serialize(s);
  }
  
  auto buffer = s.move_data();
  vds::binary_deserializer ds(buffer.data(), buffer.size());
  for(auto & p : steps) {
    p->deserialize(ds);
  }
}


/////////////////////////////////////////////////////////////////////
class message {
public:

  uint32_t field1;
  std::string field2;

  template <typename visitor_t>
  void visit(visitor_t & v) {
    v(field1, field2);
  }
};


TEST(core_tests, test_message_serialize) {
  auto data = vds::message_serialize<message>(10, "test");
  vds::binary_deserializer d(data);
  auto m1 = vds::message_deserialize<message>(d);

  GTEST_ASSERT_EQ(m1.field1, 10);
  GTEST_ASSERT_EQ(m1.field2, "test");
}
