//
// Created by vadim on 16.04.18.
//
#include "gtest/gtest.h"
#include "log_parser.h"
#include "filter_parser.h"

TEST(test_filter, test_match)
{
  filter_parser parser;

  ASSERT_TRUE(parser.parse_filter("**?test***"));
  ASSERT_TRUE(parser.is_match("sstestdd"));
  ASSERT_FALSE(parser.is_match("testdd"));
  ASSERT_TRUE(parser.is_match("stest"));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
