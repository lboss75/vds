/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "sql_builder_tests.h"
#include "service_provider.h"

vds::mock_database::mock_database()
: impl_(nullptr)
{
}

vds::mock_database::~mock_database()
{
}

void vds::mock_database::async_transaction(
  const service_provider & /*sp*/,
  const std::function<bool (vds::mock_database_transaction & t)> & callback)
{
  mock_database_transaction t{ std::shared_ptr<_database>() };
  callback(t);
}

void vds::mock_database::async_read_transaction(
  const service_provider & /*sp*/,
  const std::function<void(vds::mock_database_read_transaction & t)> & callback)
{
  mock_database_read_transaction t{ std::shared_ptr<_database>() };
  callback(t);
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

void vds::mock_sql_statement::set_parameter(int , const const_data_buffer & )
{
}

bool vds::mock_sql_statement::execute()
{
  return false;
}

static std::string result_sql;

vds::mock_sql_statement vds::mock_database_read_transaction::parse(const char * sql)
{
  result_sql = sql;
  return mock_sql_statement(nullptr);
}


TEST(sql_builder_tests, test_select) {

  vds::barrier b;
  vds::database db;
  db.async_transaction(vds::service_provider::empty(), [&b](vds::database_transaction & trans) {

    test_table1 t1;
    test_table2 t2;

    auto reader = trans.get_reader(
      t1
      .select(vds::db_max(t1.column1), t1.column2, t2.column1)
      .inner_join(t2, t1.column1 == t2.column1)
      .where(t1.column1 == 10 && t2.column2 == "test")
      .order_by(t1.column1, vds::db_desc_order(t1.column1)));

    b.set();
    return true;
  });

  b.wait();

  ASSERT_EQ(result_sql,
    "SELECT MAX(t0.column1),t0.column2,t1.column1 FROM test_table1 t0 INNER JOIN test_table2 t1 ON t0.column1=t1.column1 WHERE (t0.column1=?2) AND (t1.column2=?1) ORDER BY t0.column1,t0.column1 DESC");

  ASSERT_EQ(int_parameter_index, 2);
  ASSERT_EQ(int_parameter_value, 10);
  ASSERT_EQ(string_parameter_index, 1);
  ASSERT_EQ(string_parameter_value, "test");
}


TEST(sql_builder_tests, test_insert) {

  vds::barrier b;
  vds::database db;
  db.async_transaction(vds::service_provider::empty(), [&b](vds::database_transaction & trans) {
    test_table1 t1;

    trans.execute(
      t1.insert(t1.column1 = 10, t1.column2 = "test"));
    b.set();
    return true;
  });
  b.wait();

  ASSERT_EQ(result_sql,
    "INSERT INTO test_table1(column1,column2) VALUES (?1,?2)");

  ASSERT_EQ(int_parameter_index, 1);
  ASSERT_EQ(int_parameter_value, 10);
  ASSERT_EQ(string_parameter_index, 2);
  ASSERT_EQ(string_parameter_value, "test");
}

TEST(sql_builder_tests, test_update) {


  vds::barrier b;
  vds::database db;
  db.async_transaction(vds::service_provider::empty(), [&b](vds::database_transaction & trans) {
  test_table1 t1;

  trans.execute(
    t1.update(t1.column1 = 10, t1.column2 = "test").where(t1.column1 == 20));
  b.set();
  return true;
  });
  b.wait();

  ASSERT_EQ(result_sql,
    "UPDATE test_table1 SET column1=?2,column2=?3 WHERE column1=?1");

  ASSERT_EQ(int_parameter_index, 2);
  ASSERT_EQ(int_parameter_value, 10);
  ASSERT_EQ(string_parameter_index, 3);
  ASSERT_EQ(string_parameter_value, "test");
}

TEST(sql_builder_tests, test_insert_from) {

  vds::barrier b;
  vds::database db;
  db.async_transaction(vds::service_provider::empty(), [&b](vds::database_transaction & trans) {
  test_table1 t1;
  test_table2 t2;

  trans.execute(
    t1.insert_into(t1.column1, t1.column2)
    .from(t2, vds::db_max(t2.column1), t2.column1, vds::db_max(vds::db_length(t2.column2)))
    .where(t2.column2 == "test"));
  b.set();
  return true;
  });
  b.wait();
  ASSERT_EQ(result_sql,
     "INSERT INTO test_table1(column1,column2) SELECT MAX(t0.column1),t0.column1,MAX(LENGTH(t0.column2)) FROM test_table2 t0 WHERE t0.column2=?1");

  ASSERT_EQ(string_parameter_index, 1);
  ASSERT_EQ(string_parameter_value, "test");
}


TEST(sql_builder_tests, test_delete) {

  vds::barrier b;
  vds::database db;
  db.async_transaction(vds::service_provider::empty(), [&b](vds::database_transaction & trans) {
  test_table1 t1;

  trans.execute(
    t1.delete_if(t1.column1 == 10));
  b.set();
  return true;
  });
  b.wait();
  ASSERT_EQ(result_sql,
    "DELETE FROM test_table1 WHERE test_table1.column1=?1");
  ASSERT_EQ(int_parameter_index, 1);
  ASSERT_EQ(int_parameter_value, 10);
}
