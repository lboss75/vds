#ifndef __VDS_CMD_VDS_CMD_APP_H_
#define __VDS_CMD_VDS_CMD_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_message;
  class http_client;

  class vds_cmd_app : public console_app
  {
    using base_class = console_app;
  public:
    vds_cmd_app();

    expected<void> main(const service_provider * sp) override;
    
    void register_services(service_registrator & registrator) override;
    void register_command_line(command_line & cmd_line) override;
    bool need_demonize() override;

  private:
    command_line_set file_upload_cmd_set_;
    command_line_set file_download_cmd_set_;
    command_line_set file_sync_cmd_set_;
    command_line_set channel_list_cmd_set_;
    command_line_set channel_create_cmd_set_;

    command_line_set storage_list_cmd_set_;
    command_line_set storage_add_cmd_set_;

    command_line_value user_login_;
    command_line_value user_password_;

    command_line_value server_;

    command_line_value channel_id_;
    command_line_value message_;
    command_line_value attachment_;
    command_line_value output_folder_;

    //Sync options
    command_line_value sync_style_;
    command_line_value exclude_;

    command_line_value output_format_;

    command_line_value channel_name_;
    command_line_value channel_type_;

    //Storage
    command_line_value storage_name_;
    command_line_value storage_folder_;
    command_line_value storage_size_;


    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;

    vds::expected<std::shared_ptr<http_client>> connect(
      const service_provider * sp);

    expected<std::string> login(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client);

    expected<void> logout(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

    expected<void> upload_file(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

    expected<void> upload_file(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session,
      const filename & fn,
      const std::string & name,
      const_data_buffer file_hash);
    
    expected<void> download_file(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);
    
    expected<void> download_file(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session,
      const filename & fn,
      const std::string & file_name,
      const const_data_buffer & file_id);

    expected<void> sync_files(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

    expected<void> channel_list(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

    expected<void> channel_create(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client, 
      const std::string & session);

    expected<void> channel_list_out(
      const const_data_buffer & response);


    struct sync_file_info {
      vds::const_data_buffer object_id_;
      std::string file_name_;
      std::string mimetype_;
      size_t size_;
    };

    expected<void> sync_file(
      const service_provider* sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session,
      const filename& exists_files,
      const std::string & rel_name,
      const std::list<sync_file_info> & file_history,
      bool enable_upload);

    expected<void> storage_list(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

    expected<void> device_list_out(
      const const_data_buffer & response);

    expected<void> storage_add(
      const service_provider * sp,
      const std::shared_ptr<http_client> & client,
      const std::string & session);

  };
}

#endif // __VDS_CMD_VDS_CMD_APP_H_
