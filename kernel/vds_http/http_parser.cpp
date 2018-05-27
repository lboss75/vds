/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "http_parser.h"

std::string vds::http_parser::url_unescape(const std::string& string) {
  std::string result;
  int state = 0;
  char next_char;
  for(auto ch : string) {
    switch (state) {
    case 0:
      if('%' == ch) {
        state = 1;
        next_char = 0;
      }
      else {
        result += ch;
      }
      break;
      case 1:
        if(ch >= '0' && ch <= '9') {
          next_char = (ch - '0') << 4;
          state = 2;
        }
        else if (ch >= 'a' && ch <= 'f') {
            next_char = (ch - 'a' + 10) << 4;
            state = 2;
          }
          else if (ch >= 'A' && ch <= 'F') {
              next_char = (ch - 'A' + 10) << 4;
              state = 2;
            }
            else {
              throw std::runtime_error("Invalid string");
            }
            break;
      case 2:
        if (ch >= '0' && ch <= '9') {
          result += (char)(next_char | (ch - '0'));
          state = 0;
        }
        else if (ch >= 'a' && ch <= 'f') {
          result += (char)(next_char | (ch - 'a' + 10));
          state = 0;
        }
        else if (ch >= 'A' && ch <= 'F') {
          result += (char)(next_char | (ch - 'A' + 10));
          state = 0;
        }
        else {
          throw std::runtime_error("Invalid string");
        }
        break;
    }
  }

  return result;
}
