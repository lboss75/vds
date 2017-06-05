/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_tests.h"

TEST(chunk_tests, test_chunks) {
    int size = std::rand();
    while (size < 1 || size > 1000) {
        size = std::rand();
    }
    
    size -= (size % 3);

    uint8_t * data = new uint8_t[size];

    for (int i = 0; i < size; ++i) {
        data[i] = uint8_t(0xFF & std::rand());
    }

    int r1 = std::rand();
    while (r1 < 0 || r1 > 255)
    {
        r1 = std::rand();
    }

    int r2 = std::rand();
    while (r1 == r2 || 0 > r2 || r2 > 255)
    {
        r2 = std::rand();
    }

    int r3 = std::rand();
    while (r3 == r1 || r3 == r2 || 0 > r3 || r3 > 255)
    {
        r3 = std::rand();
    }

    vds::chunk_generator<uint8_t> g1(3, r1);
    vds::chunk_generator<uint8_t> g2(3, r2);
    vds::chunk_generator<uint8_t> g3(3, r3);
    
    vds::chunk<uint8_t> c1(g1, data, size);
    vds::chunk<uint8_t> c2(g2, data, size);
    vds::chunk<uint8_t> c3(g3, data, size);

    std::vector<uint8_t> result;
    uint8_t ns[] = { (uint8_t)r1, (uint8_t)r2, (uint8_t)r3 };
    const vds::chunk<uint8_t> * chunks[] = { &c1, &c2, &c3 };
    vds::chunk_restore<uint8_t>(3, ns).restore(result, chunks);

    ASSERT_EQ(size, result.size());
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(data[i], result[i]);
    }
}

TEST(chunk_tests, test_chunks16) {
    int size = std::rand();
    while (size < 1 || size > 1000) {
        size = std::rand();
    }
    
    size -= (size % 3);

    uint16_t * data = new uint16_t[size];

    for (int i = 0; i < size; ++i) {
        data[i] = uint16_t(0xFFFF & std::rand());
    }

    int r1 = std::rand();
    while (r1 < 0 || r1 > 255)
    {
        r1 = std::rand();
    }

    int r2 = std::rand();
    while (r1 == r2 || 0 > r2 || r2 > 255)
    {
        r2 = std::rand();
    }

    int r3 = std::rand();
    while (r3 == r1 || r3 == r2 || 0 > r3 || r3 > 255)
    {
        r3 = std::rand();
    }

    vds::chunk_generator<uint16_t> g1(3, r1);
    vds::chunk_generator<uint16_t> g2(3, r2);
    vds::chunk_generator<uint16_t> g3(3, r3);
    
    vds::chunk<uint16_t> c1(g1, data, size);
    vds::chunk<uint16_t> c2(g2, data, size);
    vds::chunk<uint16_t> c3(g3, data, size);

    std::vector<uint16_t> result;
    uint16_t ns[] = { (uint16_t)r1, (uint16_t)r2, (uint16_t)r3 };
    const vds::chunk<uint16_t> * chunks[] = { &c1, &c2, &c3 };
    vds::chunk_restore<uint16_t>(3, ns).restore(result, chunks);

    ASSERT_EQ(size, result.size());
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(data[i], result[i]);
    }
}

TEST(chunk_tests, test_chunks_storage) {
    int size = std::rand() % 2000;
    while (size < 1 || size > 2000) {
        size = std::rand();
    }
    
    //Generate test data
    uint8_t * data = new uint8_t[size];
    for (int i = 0; i < size; ++i) {
        data[i] = uint8_t(0xFF & std::rand());
    }
    
    const uint16_t horcrux_count = 1000;
    const uint16_t mix_horcrux = 800;
    vds::chunk_storage storage(vds::guid::new_guid(), mix_horcrux);
    std::list<vds::chunk_storage::horcrux> horcruxes;
    std::unordered_map<uint16_t, bool> processed_horcruxes;
    while(horcruxes.size() < mix_horcrux){
      uint16_t replica;
      for(;;) {
        replica = (uint16_t)std::rand();
        if(horcrux_count > replica
          && processed_horcruxes.end() == processed_horcruxes.find(replica)){
          break;
        }
      }
      
      vds::binary_serializer s;
      vds::ichunk_storage(&storage).generate_replica(s, 0, replica, data, size);
      
      processed_horcruxes[replica] = true;
      vds::binary_deserializer ds(s.data());
      horcruxes.push_back(vds::chunk_storage::horcrux(ds));
      ASSERT_EQ(ds.size(), 0);
    }

    vds::binary_serializer result;
    vds::ichunk_storage(&storage).restore_data(result, horcruxes);

    ASSERT_LE(size, result.size());
    for (int i = 0; i < size; ++i) {
      if(data[i] != result[i]){
        FAIL() << "data[" << i << "](" << (int)data[i] << ") != result[" << i << "](" << (int)result[i] << ")";
      }
    }
}
