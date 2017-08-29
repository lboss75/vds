/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__TEST_CONFIG_H_
#define __TEST_VDS_LIBS__TEST_CONFIG_H_

#include "targetver.h"
#include "logger.h"

class test_config
{
public:
  test_config()
  : log_level_(vds::log_level::ll_warning)
  {
  }
  
  static test_config & instance()
  {
    static test_config instance;
    return instance;
  }
  
  vds::log_level log_level() const { return this->log_level_; }
  
  void parse(int argc, char **argv)
  {
  }
  
private:
  vds::log_level log_level_;
};

#endif//__TEST_VDS_LIBS__TEST_CONFIG_H_
