/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__TEST_LOG_H_
#define __TEST_VDS_LIBS__TEST_LOG_H_

#include "targetver.h"
#include <ostream>
#include <list>
#include <map>

inline void print_value(std::ostream & logfile, const std::string & value, int width) {
  logfile << value;
  for (int i = value.length(); i < width; ++i) {
    logfile << ' ';
  }
}

inline void print_table(
  std::ostream & logfile,
  const std::list<std::tuple<std::string, std::map<std::string, std::string>>> & table) {
  std::size_t first_column_width = 0U;
  std::map<std::string, std::size_t> column_witdhs;

  //Calculate
  for (auto row : table) {
    if (first_column_width < std::get<0>(row).length()) {
      first_column_width = std::get<0>(row).length();
    }

    for (auto column : std::get<1>(row)) {
      if (column_witdhs[column.first] < column.first.length()) {
        column_witdhs[column.first] = column.first.length();
      }

      if (column_witdhs[column.first] < column.second.length()) {
        column_witdhs[column.first] = column.second.length();
      }
    }
  }
  //Out
  print_value(logfile, std::string(), first_column_width);

  for (auto column : column_witdhs) {
    logfile << '|';
    print_value(logfile, column.first, column.second);
  }
  logfile << '\n';

  for (auto row : table) {
    print_value(logfile, std::get<0>(row), first_column_width);

    for (auto column : column_witdhs) {
      logfile << '|';
      print_value(logfile, std::get<1>(row)[column.first], column.second);
    }
    logfile << '\n';
  }
}


#endif // __TEST_VDS_LIBS__TEST_LOG_H_
