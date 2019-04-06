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
  : log_level_(vds::log_level::ll_trace)
  {
  }
  
  static test_config & instance()
  {
    static test_config instance;
    return instance;
  }
  
  vds::log_level log_level() const { return this->log_level_; }
  const std::unordered_set<std::string> & modules() const { return this->modules_; }
  
  void parse(int argc, char **argv)
  {
    static char modules_arg_prefix[] = "--modules=";
    
    for(int i = 1; i < argc; ++i){
      if(0 == strncmp(argv[i], modules_arg_prefix, sizeof(modules_arg_prefix) - 1)) {
        auto modules = argv[i] + sizeof(modules_arg_prefix) - 1;
        while(0 != *modules) {
          auto p = strchr(modules, ',');
          if(nullptr == p){
            this->modules_.emplace(modules);
            break;
          }
          else {
            this->modules_.emplace(std::string(modules, p - modules));
            modules = p + 1;
          }
        }
      }
      else if(0 == strcmp(argv[i], "--trace")){
        this->log_level_ = vds::log_level::ll_trace;
      }
      else if(0 == strcmp(argv[i], "--debug")){
        this->log_level_ = vds::log_level::ll_debug;
      }
    }
  }
  
private:
  vds::log_level log_level_;
  std::unordered_set<std::string> modules_;
};

#define CHECK_EXPECTED_GTEST(v) { auto __result = (v); if(__result.has_error()) { GTEST_FATAL_FAILURE_(__result.error()->what()); }}
#define GET_EXPECTED_VALUE_GTEST(var, v) \
  {\
    auto __result = (v);\
    if(__result.has_error()) { \
      GTEST_FATAL_FAILURE_(__result.error()->what());\
    }\
    var = std::move(__result.value());\
  }

#define GET_EXPECTED_GTEST(var, v) \
  std::remove_reference<decltype((v).value())>::type var;\
  GET_EXPECTED_VALUE_GTEST(var, v);



#endif//__TEST_VDS_LIBS__TEST_CONFIG_H_
