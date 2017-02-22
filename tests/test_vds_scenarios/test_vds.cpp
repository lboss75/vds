/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "test_vds.h"
#include "vds_mock.h"

TEST(test_vds, test_initial)
{
  vds_mock mock;

  mock.start(10, 100);
}