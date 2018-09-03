/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_SCENARIOS_VDS_MOCK_H_
#define __TEST_VDS_SCENARIOS_VDS_MOCK_H_

#include "network_service.h"
#include "dht_network.h"
#include "user_channel.h"

namespace vds {
  class user_manager;
}

class mock_server
{
public:
  mock_server(int index, int udp_port);

  void start();
  void stop();

  void init_root(
    const std::string &root_user_name,
    const std::string &root_password) const;
  //void allocate_storage(
  //  const std::string& root_login,
  //  const std::string& root_password);
  static void init(
	  int index, int udp_port, const std::string & user_login, const std::string & user_password);

  template <typename T>
  T * get() const {
    return this->sp_.get<T>();
  }

  const vds::service_provider & get_service_provider() const {
    return this->sp_;
  }

  vds::async_task<vds::server_statistic> get_statistic() const{
	  auto scope = this->sp_.create_scope(__FUNCTION__);
	  vds::mt_service::enable_async(scope);
    return this->server_.get_statistic(scope);
  }
  
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

  void login(
    const std::string& root_login,
    const std::string& root_password,
    const std::function<void(const vds::service_provider & sp, const std::shared_ptr<vds::user_manager> & user_mng)> & callback);

};

class vds_mock
{
public:
  vds_mock();
  ~vds_mock();

  void start(size_t server_count);
  void stop();

  void allow_write_channel(
      size_t client_index,
      const vds::const_data_buffer &channel_id);

  void allow_read_channel(
      size_t client_index,
      const vds::const_data_buffer &channel_id);

  vds::const_data_buffer upload_file(
      size_t client_index,
      const vds::const_data_buffer &channel_id,
      const std::string &name,
      const std::string &mimetype,
      const std::shared_ptr<vds::continuous_buffer<uint8_t>> & input_stream);

  vds::async_task<
      std::string /*content_type*/,
      size_t /*body_size*/,
      std::shared_ptr<vds::continuous_buffer<uint8_t>> /*output_stream*/>
  download_data(
	  size_t client_index,
	  const vds::const_data_buffer & channel_id,
	  const std::string & name,
	  const vds::const_data_buffer & file_hash);

  bool dump_statistic(std::ostream & logfile, std::vector<vds::server_statistic>& statistics);
  void sync_wait();

  vds::user_channel create_channel(int index, const std::string &name);

  vds::service_provider get_sp(int client_index);

private:
  std::vector<std::unique_ptr<mock_server>> servers_;

  std::string root_login_;
  std::string root_password_;

  static std::string generate_password(size_t min_len = 4, size_t max_len = 20);
};

#endif//__TEST_VDS_SCENARIOS_VDS_MOCK_H_
