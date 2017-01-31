#ifndef __VDS_PARSER_PARSE_ERROR_H_
#define __VDS_PARSER_PARSE_ERROR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <exception>
#include <string>

#ifdef _WIN32
#define _GLIBCXX_USE_NOEXCEPT
#endif

namespace vds {
  class parse_error : public std::runtime_error
  {
  public:
    parse_error(
      const std::string & stream_name,
      int line,
      int column,
      const std::string & message
    );
  };
}

#endif // __VDS_PARSER_PARSE_ERROR_H_
