/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_url_parser.h"

TEST(test_url_parser, test_parser)
{
  auto result = vds::url_parser::parse_network_address("https://server:port");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "");

  result = vds::url_parser::parse_network_address("https://server:port/");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/");

  result = vds::url_parser::parse_network_address("https://server");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "");

  result = vds::url_parser::parse_network_address("https://:port");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "");

  result = vds::url_parser::parse_network_address("https://server/");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/");

  result = vds::url_parser::parse_network_address("https://:port/");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/");


  result = vds::url_parser::parse_network_address("https://server/path");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/path");

  result = vds::url_parser::parse_network_address("https://:port/path");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/path");

  result = vds::url_parser::parse_network_address("https://server:port/path");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/path");

  result = vds::url_parser::parse_network_address("https://server/path:test");
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/path:test");
}
