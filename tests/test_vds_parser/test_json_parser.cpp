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

class test_json_parser_validate
{
public:

  template<typename context_type>
  class handler : public vds::sequence_step<context_type, void(void)>
  {
    using base_class = vds::sequence_step<context_type, void(void)>;
  public:
    handler(
      const context_type & context,
      const test_json_parser_validate & args
    ) : base_class(context)
    {
    }

    void operator()(vds::json_value * root)
    {
      auto root_object = dynamic_cast<vds::json_object *>(root);
      ASSERT_NE(nullptr, root_object);

      int prop_count = 0;
      root_object->visit(
        [&prop_count](const vds::json_property & prop) {
        ++prop_count;
        if ("glossary" == prop.name()) {
          auto glossary_object = dynamic_cast<vds::json_object *>(prop.value());
          ASSERT_NE(nullptr, glossary_object);
          int glossary_prop_count = 0;
          glossary_object->visit(
            [&glossary_prop_count](const vds::json_property & prop) {
            ++glossary_prop_count;
            if ("title" == prop.name()) {
              auto title_prop = dynamic_cast<vds::json_primitive *>(prop.value());
              ASSERT_NE(nullptr, title_prop);
              ASSERT_EQ("example glossary", title_prop->value());
            }
            else if ("GlossDiv" == prop.name()) {
              auto glossdiv_prop = dynamic_cast<vds::json_object *>(prop.value());
              ASSERT_NE(nullptr, glossdiv_prop);

            }
            else {
              FAIL() << "Invalid property " << prop.name();

            }
          }
          );
        }
        else {
          FAIL() << "Invalid property " << prop.name();
        }
      }
      );

      ASSERT_EQ(1, prop_count);

      //this->prev();
    }
  };
};

TEST(test_json_parser, test_parser) {
  vds::sequence(
    vds::json_parser("test"),
    test_json_parser_validate()
  )
  (
    []() {},
    [](std::exception * ex) { FAIL() << ex->what(); },
   test_data,
   sizeof(test_data) - 1
  );
}
