/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_SCENARIOS_VDS_MOCK_H_
#define __TEST_VDS_SCENARIOS_VDS_MOCK_H_

class mock_server
{
public:

};

class mock_client
{
public:
  static void init_root(int index, const std::string & root_password, int port);

};

class vds_mock
{
public:
  void start(int client_count, int server_count);

private:
  std::list<std::unique_ptr<mock_client>> clients_;


  static std::string generate_password(size_t min_len = 4, size_t max_len = 20);

};

#endif//__TEST_VDS_SCENARIOS_VDS_MOCK_H_