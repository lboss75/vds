/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_json_parser.h"
#include "../test_libs/test_config.h"


static const char test_data[] =
"\
  {\
    \"glossary\": {\
      \"title\": \"example glossary\",\
        \"GlossDiv\" : {\
        \"title\": \"S\",\
          \"GlossList\" : {\
          \"GlossEntry\": {\
            \"ID\": \"SGML\",\
              \"SortAs\" : \"SGML\",\
              \"GlossTerm\" : \"Standard Generalized Markup Language\",\
              \"Acronym\" : \"SGML\",\
              \"Abbrev\" : \"ISO 8879:1986\",\
              \"GlossDef\" : {\
              \"para\": \"A meta-markup language, used to create markup languages such as DocBook.\",\
                \"GlossSeeAlso\" : [\"GML\", \"XML\"]\
            },\
              \"GlossSee\" : \"markup\"\
          }\
        }\
      }\
    }\
  }\
";


//static const char test_data[] = "{\"glossary\":\"\"}";

static vds::expected<bool> test_json_parser_validate(const std::shared_ptr<vds::json_value> & value)
{
  auto root_object = std::dynamic_pointer_cast<vds::json_object>(value);
  if(nullptr == root_object) {
    return vds::make_unexpected<std::runtime_error>("Test test");
  }

  int prop_count = 0;
  root_object->visit(
    [&prop_count](const std::shared_ptr<vds::json_property> & prop) {
    ++prop_count;
    if ("glossary" == prop->name()) {
      auto glossary_object = std::dynamic_pointer_cast<vds::json_object>(prop->value());
      ASSERT_NE(true, !glossary_object);
      int glossary_prop_count = 0;
      glossary_object->visit(
        [&glossary_prop_count](const std::shared_ptr<vds::json_property> & prop) {
        ++glossary_prop_count;
        if ("title" == prop->name()) {
          auto title_prop = std::dynamic_pointer_cast<vds::json_primitive>(prop->value());
          ASSERT_NE(true, !title_prop);
          ASSERT_EQ("example glossary", title_prop->value());
        }
        else if ("GlossDiv" == prop->name()) {
          auto glossdiv_prop = std::dynamic_pointer_cast<vds::json_object>(prop->value());
          GTEST_ASSERT_EQ(true, nullptr != glossdiv_prop.get());

        }
        else {
          FAIL() << "Invalid property " << prop->name();

        }
      }
      );
    }
    else {
      FAIL() << "Invalid property " << prop->name();
    }
  }
  );

  if(1 != prop_count){
    return vds::make_unexpected<std::runtime_error>("test failed");
  }

  return true;
}

TEST(test_json_parser, test_parser) {
  auto parser = std::make_shared<vds::json_parser>("test",
    [](const std::shared_ptr<vds::json_value> & value) -> vds::expected<void>{
      CHECK_EXPECTED(test_json_parser_validate(value));
      return vds::expected<void>();
    }
  );
  CHECK_EXPECTED_GTEST(parser->write_async((const uint8_t *)test_data, sizeof(test_data) - 1).get());
  CHECK_EXPECTED_GTEST(parser->write_async(nullptr, 0).get());
}
