#ifndef __VDS_CORE_STRING_UTILS_H_
#define __VDS_CORE_STRING_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include <string>

namespace vds {

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

  static inline std::list<std::string> split_string(const std::string & s, char ch, bool trim_items, bool allow_empty = false) {
    std::list<std::string> result;
    std::string::size_type start = 0;
    for (;;) {
      auto p = s.find(ch, start);
      if (std::string::npos == p) {
        auto value = s.substr(start);
        if (trim_items) {
          trim(value);
        }

        if (allow_empty || !value.empty()) {
          result.push_back(value);
        }
        break;
      }
      else {
        auto value = s.substr(start, p - start);
        start = p + 1;
        if (trim_items) {
          trim(value);
        }

        if (allow_empty || !value.empty()) {
          result.push_back(value);
        }
      }
    }

    return result;
  }
}
#endif//__VDS_CORE_STRING_UTILS_H_
