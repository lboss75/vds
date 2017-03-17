/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "gf_tests.h"

TEST(gf_tests, test_mul) {
	static uint8_t original[8][8] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 1, 2, 3, 4, 5, 6, 7 },
		{ 0, 2, 4, 6, 3, 1, 7, 5 },
		{ 0, 3, 6, 5, 7, 4, 1, 2 },
		{ 0, 4, 3, 7, 6, 2, 5, 1 },
		{ 0, 5, 1, 4, 2, 7, 3, 6 },
		{ 0, 6, 7, 1, 5, 3, 2, 4 },
		{ 0, 7, 5, 2, 1, 6, 4, 3 }
	};

	for (uint8_t i = 1; i < 8; ++i) {
		for (uint8_t j = 1; j < 8; ++j) {
			uint8_t left[1] = { i };
			uint8_t right[1] = { j };
			uint8_t result[1] = { original[i][j] };

			vds::gf<3> r(result);
			vds::gf<3> m(vds::gf<3>(left) * vds::gf<3>(right));
// 			printf("%s * %s = %s, expected %s\n",
// 				vds::gf<3>(left).toString().c_str(),
// 				vds::gf<3>(right).toString().c_str(),
// 				m.toString().c_str(),
// 				r.toString().c_str()
// 				);
			ASSERT_EQ(r, m);
		}
	}
}


TEST(gf_tests, test_math) {
    vds::gf_math<u_int8_t> math;

    for (int i = 0; i < 1000; ++i) {
        uint8_t x = std::rand() & 0xFF;
        uint8_t y = std::rand() & 0xFF;
        uint8_t gx[] = { x };
        uint8_t gy[] = { y };
        uint8_t z = (vds::gf<8>(gx) * vds::gf<8>(gy)).data()[0];
        uint8_t p = (vds::gf<8>(gx) + vds::gf<8>(gy)).data()[0];

        ASSERT_EQ(math.mul(x, y), z);
        ASSERT_EQ(math.mul(y, x), z);
        
        if (0 != z) {
          ASSERT_EQ(math.div(z, x), y);
          ASSERT_EQ(math.div(z, y), x);
        }

        ASSERT_EQ(math.add(x, y), p);
        ASSERT_EQ(math.add(y, x), p);
        ASSERT_EQ(math.sub(p, x), y);
        ASSERT_EQ(math.sub(p, y), x);
    }
}