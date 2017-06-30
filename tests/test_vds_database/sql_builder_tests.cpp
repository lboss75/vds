/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "sql_builder_tests.h"

vds::mock_database::mock_database()
: impl_(nullptr)
{
}

vds::mock_database::~mock_database()
{
}

vds::mock_database_transaction vds::mock_database::begin_transaction()
{
  return mock_database_transaction();
}

vds::mock_sql_statement::mock_sql_statement(_sql_statement * )
{
}

vds::mock_sql_statement::mock_sql_statement(mock_sql_statement && )
{
}

vds::mock_sql_statement::~mock_sql_statement()
{
}

static int int_parameter_index;
static int int_parameter_value;

void vds::mock_sql_statement::set_parameter(int index, int value)
{
  int_parameter_index = index;
  int_parameter_value = value;
}

void vds::mock_sql_statement::set_parameter(int , uint64_t )
{
}

static int string_parameter_index;
static std::string string_parameter_value;

void vds::mock_sql_statement::set_parameter(int index, const std::string & value)
{
  string_parameter_index = index;
  string_parameter_value = value;
}

void vds::mock_sql_statement::set_parameter(int , const guid & )
{
}

void vds::mock_sql_statement::set_parameter(int , const const_data_buffer & )
{
}

bool vds::mock_sql_statement::execute()
{
  return false;
}

static std::string result_sql;

vds::mock_sql_statement vds::mock_database_transaction::parse(const std::string & sql)
{
  result_sql = sql;
  return mock_sql_statement(nullptr);
}


TEST(sql_builder_tests, test_select) {

  test_table1 t1;
  test_table2 t2;

  vds::database db;
  vds::database_transaction trans = db.begin_transaction();

  auto reader = trans.select(vds::db_max(t1.column1), t1.column2, t2.column1)
  .from(t1)
  .inner_join(t2, t1.column1 == t2.column1)
  .where(t1.column1 == 10 && t2.column2 == "test")
  .get_reader();

  ASSERT_EQ(result_sql,
    "SELECT MAX(t0.column1),t0.column2,t1.column1 FROM test_table1 t0 INNER JOIN test_table2 t1 ON t0.column1=t1.column1 WHERE (t0.column1=@p2) AND (t1.column2=@p1)");
  ASSERT_EQ(int_parameter_index, 1);
  ASSERT_EQ(int_parameter_value, 10);
  ASSERT_EQ(string_parameter_index, 0);
  ASSERT_EQ(string_parameter_value, "test");
}


TEST(sql_builder_tests, test_insert) {

  test_table1 t1;

  vds::database db;
  vds::database_transaction trans = db.begin_transaction();
  
  trans
    .insert_into(t1)
    .set(t1.column1 = 10, t1.column2 = "test")
    .execute();
    
  ASSERT_EQ(result_sql,
    "INSERT INTO test_table1(column1,column2) VALUES (@p0,@p1)");
  
  ASSERT_EQ(int_parameter_index, 0);
  ASSERT_EQ(int_parameter_value, 10);
  ASSERT_EQ(string_parameter_index, 1);
  ASSERT_EQ(string_parameter_value, "test");
}

TEST(sql_builder_tests, test_delete) {

  test_table1 t1;

  vds::database db;
  vds::database_transaction trans = db.begin_transaction();
  
  trans
    .delete_from(t1)
    .where(t1.column1 == 10)
    .execute();
    
  ASSERT_EQ(result_sql,
    "DELETE FROM test_table1 WHERE test_table1.column1=@p1");
  ASSERT_EQ(int_parameter_index, 0);
  ASSERT_EQ(int_parameter_value, 10);
}
