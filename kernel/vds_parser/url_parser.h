#ifndef __VDS_PARSER_URL_PARSER_H_
#define __VDS_PARSER_URL_PARSER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class url_parser
  {
  public:

    static bool parse_addresses(
      const std::string & addresses,
      const std::function<bool(const std::string & protocol, const std::string & address)> & handler);

    static bool parse_address(
      const std::string & address,
      const std::function<bool(const std::string & protocol, const std::string & address)> & handler);


    struct http_address
    {
      std::string protocol;
      std::string server;
      std::string port;
      std::string path;
    };

    static http_address parse_http_address(
      const std::string & address);
  };
}

#endif // __VDS_PARSER_URL_PARSER_H_
