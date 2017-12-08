#ifndef __VDS_CORE_STRING_FORMAT_H_
#define __VDS_CORE_STRING_FORMAT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sstream> 
#include <string>
#include <memory>

namespace vds {

#ifndef _WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

  template<typename ... Args>
  inline std::string string_format(const std::string& format, Args ... args) {
    size_t size = std::snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), size - 1);
  }

#ifndef _WIN32
#pragma GCC diagnostic pop
#endif

  static inline void replace_string(std::string & result, const std::string & original, const std::string & target) {
    size_t index = 0;
    for (;;) {
      index = result.find(original, index);
      if (index == std::string::npos) {
        break;
      }
      result.replace(index, original.length(), target);
      index += target.length();
    }
  }

  static inline std::string replace_string(const std::string & str, const std::string & original, const std::string & target) {
    std::string result = str;
    replace_string(result, original, target);
    return result;
  }

}

#endif//__VDS_CORE_STRING_FORMAT_H_
