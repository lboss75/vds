/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "encoding.h"

std::wstring vds::utf16::from_utf8(const std::string & original)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
  return convert.from_bytes(original);
}

wchar_t vds::utf8::next_char(const char *& utf8string, size_t & len)
{
  if (len == 0) {
    return 0;
  }

  wchar_t result;
  if (0 == (0b10000000 & (uint8_t)*utf8string)) {
    result = (wchar_t)*utf8string++;
    --len;
  }
  else if (0b11000000 == (0b11100000 & (uint8_t)*utf8string)) {
    if (len < 2 || (0b10000000 != (0b11000000 & (uint8_t)utf8string[1]))) {
      throw new std::runtime_error("Invalid UTF8 string");
    }

    result = (wchar_t)(0x80 + ((0b00011111 & (uint8_t)utf8string[0]) << 6) | (0b00111111 & (uint8_t)utf8string[1]));
    utf8string += 2;
    len += 2;
  }
  else if (0b11100000 == (0b11110000 & (uint8_t)*utf8string)) {
    if (len < 3
      || (0b10000000 != (0b11000000 & (uint8_t)utf8string[1]))
      || (0b10000000 != (0b11000000 & (uint8_t)utf8string[2]))) {
      throw new std::runtime_error("Invalid UTF8 string");
    }

    result = (wchar_t)(0x800
      + ((0b00011111 & (uint8_t)utf8string[0]) << 12)
        | ((0b00111111 & (uint8_t)utf8string[1]) << 6)
        | (0b00111111 & (uint8_t)utf8string[2])
      );
    utf8string += 3;
    len += 3;
  }
  else if (0b11110000 == (0b11111000 & (uint8_t)*utf8string)) {
    if (len < 4
      || (0b10000000 != (0b11000000 & (uint8_t)utf8string[1]))
      || (0b10000000 != (0b11000000 & (uint8_t)utf8string[2]))
      || (0b10000000 != (0b11000000 & (uint8_t)utf8string[3]))) {
      throw new std::runtime_error("Invalid UTF8 string");
    }

    result = (wchar_t)(0x10000
      + ((0b00011111 & (uint8_t)utf8string[0]) << 18)
      | ((0b00111111 & (uint8_t)utf8string[1]) << 12)
      | ((0b00111111 & (uint8_t)utf8string[2]) << 6)
      | (0b00111111 & (uint8_t)utf8string[3])
      );
    utf8string += 4;
    len += 4;
  }
  else {
    throw new std::runtime_error("Invalid UTF8 string");
  }
}
