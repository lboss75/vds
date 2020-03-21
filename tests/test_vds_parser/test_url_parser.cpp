/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_url_parser.h"
#include "test_config.h"

TEST(test_url_parser, test_parser)
{
  GET_EXPECTED_GTEST(result, vds::url_parser::parse_network_address("https://server:port"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server:port/"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://:port"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server/"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://:port/"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/");
  
  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server/path"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/path");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://:port/path"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/path");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server:port/path"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "port");
  ASSERT_EQ(result.path, "/path");

  GET_EXPECTED_VALUE_GTEST(result, vds::url_parser::parse_network_address("https://server/path:test"));
  ASSERT_EQ(result.protocol, "https");
  ASSERT_EQ(result.server, "server");
  ASSERT_EQ(result.port, "");
  ASSERT_EQ(result.path, "/path:test");
}
