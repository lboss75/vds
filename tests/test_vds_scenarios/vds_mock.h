/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_SCENARIOS_VDS_MOCK_H_
#define __TEST_VDS_SCENARIOS_VDS_MOCK_H_

class mock_server
{
public:
  mock_server(int index, int port);

  void start();
  voif stop();

private:
  int index_;
  int port_;

  vds::service_registrator registrator_;
  vds::network_service network_service_;
  vds::console_logger console_logger_;
  vds::server server_;
  vds::task_manager task_manager_;
  vds::crypto_service crypto_service_;


  void start_vds(const std::function<void(const vds::service_provider & sp)> & handler);
};

class mock_client
{
public:
  mock_client(int index);

  void init_root(const std::string & root_password, int port);

private:
  int index_;

  void start_vds(const std::function<void(const vds::service_provider & sp)> & handler);
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