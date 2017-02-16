#ifndef __VDS_CORE_ENCODING_H_
#define __VDS_CORE_ENCODING_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class utf8
  {
  public:
    static wchar_t next_char(const char *& utf8string, size_t & len);

  };

  class utf16
  {
  public:
    static std::wstring from_utf8(const std::string & original);
  };
}

#endif//__VDS_CORE_ENCODING_H_
