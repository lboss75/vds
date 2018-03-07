#ifndef __VDS_CORE_STRING_FORMAT_H_
#define __VDS_CORE_STRING_FORMAT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <sstream> 
#include <string>
#include <memory>
#include <algorithm> 
#include <cctype>
#include <locale>

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

  static inline std::string display_string(const std::string & str, size_t first_symbols = 10, size_t last_symbols = 10) {
	  if (str.length() < first_symbols + last_symbols + 3) {
		  return str;
	  }
	  return str.substr(0, first_symbols) + "..." + str.substr(str.length() - last_symbols - 1, last_symbols);
  }

  // trim from start (in place)
  static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
      std::not1(std::ptr_fun<int, int>(std::isspace))));
  }

  // trim from end (in place)
  static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
      std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
  }

  // trim from both ends (in place)
  static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
  }

  // trim from start (copying)
  static inline std::string ltrim_copy(std::string s) {
    ltrim(s);
    return s;
  }

  // trim from end (copying)
  static inline std::string rtrim_copy(std::string s) {
    rtrim(s);
    return s;
  }

  // trim from both ends (copying)
  static inline std::string trim_copy(std::string s) {
    trim(s);
    return s;
  }

}

#endif//__VDS_CORE_STRING_FORMAT_H_
