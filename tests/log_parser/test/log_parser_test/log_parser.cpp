#include "stdafx.h"

TEST(test_filter, test_empty)
{
  filter_statemachine statemachine;

  ASSERT_TRUE(filter_parser::parse_filter("*", statemachine));

  ASSERT_TRUE(statemachine.is_match("sstestdd"));
  ASSERT_TRUE(statemachine.is_match("testdd"));
  ASSERT_TRUE(statemachine.is_match("stest"));
  ASSERT_TRUE(statemachine.is_match(""));

  ASSERT_TRUE(filter_parser::parse_filter("", statemachine));

  ASSERT_FALSE(statemachine.is_match("sstestdd"));
  ASSERT_FALSE(statemachine.is_match("testdd"));
  ASSERT_FALSE(statemachine.is_match("stest"));
  ASSERT_TRUE(statemachine.is_match(""));

  ASSERT_TRUE(filter_parser::parse_filter("?", statemachine));

  ASSERT_FALSE(statemachine.is_match("sstestdd"));
  ASSERT_FALSE(statemachine.is_match("testdd"));
  ASSERT_FALSE(statemachine.is_match("stest"));
  ASSERT_FALSE(statemachine.is_match(""));
  ASSERT_TRUE(statemachine.is_match("*"));

  ASSERT_TRUE(filter_parser::parse_filter("?*", statemachine));

  ASSERT_TRUE(statemachine.is_match("sstestdd"));
  ASSERT_TRUE(statemachine.is_match("testdd"));
  ASSERT_TRUE(statemachine.is_match("stest"));
  ASSERT_FALSE(statemachine.is_match(""));
  ASSERT_TRUE(statemachine.is_match("*"));

}

TEST(test_filter, test_match)
{
  filter_statemachine statemachine;
  ASSERT_TRUE(filter_parser::parse_filter("**?test***", statemachine));

  ASSERT_TRUE(statemachine.is_match("sstestdd"));
  ASSERT_FALSE(statemachine.is_match("testdd"));
  ASSERT_TRUE(statemachine.is_match("stest"));

  ASSERT_TRUE(filter_parser::parse_filter("**?test", statemachine));

  ASSERT_FALSE(statemachine.is_match("sstestdd"));
  ASSERT_FALSE(statemachine.is_match("testdd"));
  ASSERT_TRUE(statemachine.is_match("stest"));

  ASSERT_TRUE(filter_parser::parse_filter("**form?signin**", statemachine));

  ASSERT_FALSE(statemachine.is_match("2018/04/7 10:55.56 INFO   [UserMng   ] Create root user BB779ED5-132C-4B5A-9CA3-4EB8003D8D13. Cert F4DE717B-4388-40C8-8C17-AE1FB2904028 | [1]application"));
  ASSERT_FALSE(statemachine.is_match("2018/04/7 10:55.56 INFO  form [UserMng   ] Create root user BB779ED5-132C-4B5A-9CA3-4EB8003D8D13. Cert F4DE717B-4388-40C8-8C17-AE1FB2signin904028 | [1]application"));
  ASSERT_TRUE(statemachine.is_match("2018/04/7 10:55.56 INFO  form [UserMng   ] Create root user BB779ED5-132C-4B5A-9CA3-4EB8003D8D13. Cert F4DE717B-4388-40C8-8C17-AE1FBform2signin904028 | [1]application"));
  ASSERT_FALSE(statemachine.is_match("2018/04/7 10:55.56 INFO  form [UserMng   ] Create root user BB779ED5-132C-4B5A-9CA3-4EB8003D8D13. Cert F4DE717B-4388-40C8-8C17-AE1FB2si gnin904028 | [1]application"));
}

