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

    static expected<bool> parse_addresses(
      const std::string & addresses,
      const std::function<expected<bool>(const std::string & protocol, const std::string & address)> & handler);

    static expected<bool> parse_address(
      const std::string & address,
      const std::function<expected<bool>(const std::string & protocol, const std::string & address)> & handler);


    struct network_address
    {
      std::string protocol;
      std::string server;
      std::string port;
      std::string path;
    };

    static network_address parse_network_address(
      const std::string & address);
  };
}

#endif // __VDS_PARSER_URL_PARSER_H_
