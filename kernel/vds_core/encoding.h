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
    static std::string to_utf8(const std::wstring & original);
  };
  
  class base64
  {
  public:
    static std::string from_bytes(const void * data, size_t len);
    static void to_bytes(const std::string & data, std::vector<uint8_t> & result);
  };
}

#endif//__VDS_CORE_ENCODING_H_
