/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_json_parser.h"


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

static size_t test_json_parser_validate(const std::shared_ptr<vds::json_value> & value)
{
  auto root_object = std::dynamic_pointer_cast<vds::json_object>(value);
  if(nullptr == root_object) {
    throw std::runtime_error("Test test");
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
          ASSERT_NE(nullptr, glossdiv_prop);

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
    throw std::runtime_error("test failed");
  }

  return true;
}

TEST(test_json_parser, test_parser) {
  vds::json_parser parser("test",
    [](const std::shared_ptr<vds::json_value> & value){
      test_json_parser_validate(value);
    }
  );
  parser.update((const char *)test_data, sizeof(test_data) - 1);
  parser.update(nullptr, 0);
}
