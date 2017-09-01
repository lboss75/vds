/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_SCENARIOS_VDS_MOCK_H_
#define __TEST_VDS_SCENARIOS_VDS_MOCK_H_

#include "network_service.h"

class mock_server
{
public:
  mock_server(int index, int tcp_port, int udp_port);

  void start();
  void stop();

  void init_root(const std::string & root_password, int tcp_port, int udp_port);
  vds::guid last_log_record() const;
  
private:
  int index_;
  int tcp_port_;
  int udp_port_;
  vds::service_provider sp_;

  vds::service_registrator registrator_;
  vds::mt_service mt_service_;
  vds::network_service network_service_;
  vds::file_logger logger_;
  vds::task_manager task_manager_;
  vds::crypto_service crypto_service_;
  vds::server server_;
  vds::connection_manager connection_manager_;
  vds::server_log_sync server_log_sync_;
};

class mock_client
{
public:
  mock_client(int index);

  void upload_file(
    const std::string & login,
    const std::string & password,
    const std::string & name,
    const void * data,
    size_t data_size);

  vds::const_data_buffer download_data(
    const std::string & login,
    const std::string & password,
    const std::string & name);

  void init_server(const std::string & root_password, const std::string & address, int tcp_port, int udp_port);
  
private:
  int index_;

  void start_vds(bool full_client, const std::function<void(const vds::service_provider & sp)> & handler, bool clear_folder);
};

class vds_mock
{
public:
  vds_mock();
  ~vds_mock();

  void start(size_t server_count);
  void stop();

  void upload_file(size_t client_index, const std::string & name, const void * data, size_t data_size);
  vds::const_data_buffer download_data(size_t client_index, const std::string & name);
  
  void sync_wait();

private:
  std::vector<std::unique_ptr<mock_server>> servers_;

  std::string root_password_;

  static std::string generate_password(size_t min_len = 4, size_t max_len = 20);
};

#endif//__TEST_VDS_SCENARIOS_VDS_MOCK_H_