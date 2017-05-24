/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"

int main(int argc, char **argv) {
  std::srand(unsigned(std::time(0)));
  setlocale(LC_ALL, "Russian");
  ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

