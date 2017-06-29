/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "sql_builder_tests.h"

TEST(sql_builder_tests, test_select) {

  test_table1 t1;
  test_table2 t2;

  vds::database db;
  vds::database_transaction trans = db.begin_transaction();

  auto reader = trans.select(t1.column1, t1.column2, t1.column1).where(t1.column1 == 10 && t2.column2 == "test").get_reader();

  //ASSERT_EQ(trans.sql, "SELECT");
}

