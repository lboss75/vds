/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "encoding.h"

std::wstring vds::utf16::from_utf8(const std::string & original)
{
  std::wstring_convert<std::codecvt<wchar_t, char, std::mbstate_t>, wchar_t> convert;
  return convert.from_bytes(original);
}
