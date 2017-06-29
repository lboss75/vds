/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __VDS_TEST_DATABASE_SQL_BUILDER_TESTS_H_
#define __VDS_TEST_DATABASE_SQL_BUILDER_TESTS_H_

#define database mock_database
#define database_transaction mock_database_transaction
#define sql_statement mock_sql_statement

#include "database_orm.h"


class test_table1 : public vds::database_table
{
public:
  test_table1()
    : database_table("test_table1"),
    column1(this, "column1"),
    column2(this, "column2")
  {
  }

  vds::database_column<int> column1;
  vds::database_column<std::string> column2;


};

class test_table2 : public vds::database_table
{
public:
  test_table2()
    : database_table("test_table2"),
    column1(this, "column1"),
    column2(this, "column2")
  {
  }

  vds::database_column<int> column1;
  vds::database_column<std::string> column2;


};


#endif//__VDS_TEST_DATABASE_SQL_BUILDER_TESTS_H_
