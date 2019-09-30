/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_SERVERS_VDS_MOCK_H_
#define __TEST_VDS_SERVERS_VDS_MOCK_H_

#include "network_service.h"
#include "dht_network.h"
#include "user_channel.h"
#include "async_buffer.h"
#include "file_operations.h"

namespace vds {
  class user_manager;
}

class mock_server
{
public:
  mock_server(
    int index,
    int udp_port,
    bool allow_network,
    bool disable_timers);
  
  ~mock_server();

  vds::expected<void> start();
  vds::expected<void> stop();

  vds::expected<void> init_root(
    const std::string &root_user_name,
    const std::string &root_password) const;

  vds::expected<void> allocate_storage(
    const std::string& root_login,
    const std::string& root_password);

  static vds::expected<void> init(
	  int index, int udp_port, const std::string & user_login, const std::string & user_password);

  template <typename T>
  T * get() const {
    return this->sp_->get<T>();
  }

  const vds::service_provider * get_service_provider() const {
    return this->sp_;
  }

  vds::async_task<vds::expected<vds::server_statistic>> get_statistic() const;
  
private:
  int index_;
  int tcp_port_;
  int udp_port_;
  vds::service_provider * sp_;

  vds::service_registrator registrator_;
  vds::mt_service mt_service_;
  vds::network_service network_service_;
  vds::file_logger logger_;
  vds::task_manager task_manager_;
  vds::crypto_service crypto_service_;
  vds::server server_;
  bool allow_network_;
  bool disable_timers_;

  vds::expected<void> login(
    const std::string& root_login,
    const std::string& root_password,
    const std::function<void( const std::shared_ptr<vds::user_manager> & user_mng)> & callback);

};

class vds_mock
{
public:
  vds_mock();
  ~vds_mock();

  vds::expected<void> start(size_t server_count, bool allow_network);
  vds::expected<void> stop();

  void allow_write_channel(
      size_t client_index,
      const vds::const_data_buffer &channel_id);

  void allow_read_channel(
      size_t client_index,
      const vds::const_data_buffer &channel_id);

  vds::expected<vds::const_data_buffer> upload_file(
      size_t client_index,
      const vds::const_data_buffer &channel_id,
      const std::string &name,
      const std::string &mimetype,
    const std::shared_ptr<vds::stream_input_async<uint8_t>>& input_stream);

  vds::async_task<vds::expected<vds::file_manager::file_operations::download_result_t>>
  download_data(
	  size_t client_index,
	  vds::const_data_buffer channel_id,
	  std::string name,
	  vds::const_data_buffer file_hash);

  vds::expected<bool> dump_statistic(std::ostream & logfile, std::vector<vds::server_statistic>& statistics);
  vds::expected<void> sync_wait();
  vds::expected<void> dump_log();

  vds::expected<vds::user_channel> create_channel(int index, const std::string & channel_type, const std::string &name);

  const vds::service_provider * get_sp(int client_index);

  vds::expected<void> init_root(size_t index);
  void disable_timers() {
    this->disable_timers_ = true;
  }

  vds::expected<void> allocate_storage();

  const std::string & root_login() const {
    return this->root_login_;
  }

  const std::string & root_password() const {
    return this->root_password_;
  }

private:
  std::vector<std::unique_ptr<mock_server>> servers_;

  std::string root_login_;
  std::string root_password_;
  bool disable_timers_ = false;

  static std::string generate_password(size_t min_len = 4, size_t max_len = 20);
};

#endif//__TEST_VDS_SERVERS_VDS_MOCK_H_
