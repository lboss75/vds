/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "test_vds.h"
#include "vds_mock.h"

TEST(test_vds, test_initial)
{
  try{
    vds_mock mock;

    mock.start(10, 100);
  }
  catch(std::exception * ex){
    FAIL() << std::unique_ptr<std::exception>(ex)->what();
  }
}