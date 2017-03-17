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
    
    const vds::chunk_generator<uint8_t> * gs[] = { &g1, &g2, &g3 };

    vds::chunk<u_int8_t> c1(g1, data, size);
    vds::chunk<u_int8_t> c2(g2, data, size);
    vds::chunk<u_int8_t> c3(g3, data, size);

    std::vector<uint8_t> result;
    uint8_t ns[] = { (uint8_t)r1, (uint8_t)r2, (uint8_t)r3 };
    const vds::chunk<uint8_t> * chunks[] = { &c1, &c2, &c3 };
    vds::chunk_restore<uint8_t>(3, ns).restore(result, chunks);

    ASSERT_EQ(size, result.size());
    for (int i = 0; i < size; ++i) {
      ASSERT_EQ(data[i], result[i]);
    }
}
