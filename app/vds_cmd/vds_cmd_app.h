#ifndef __VDS_CMD_VDS_CMD_APP_H_
#define __VDS_CMD_VDS_CMD_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"

namespace vds {
  class http_message;

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
    command_line_set channel_list_cmd_set_;
    command_line_set channel_create_cmd_set_;

    command_line_value user_login_;
    command_line_value user_password_;

    command_line_value server_;

    command_line_value channel_id_;
    command_line_value message_;
    command_line_value attachment_;
    command_line_value output_folder_;

    command_line_value output_format_;

    command_line_value channel_name_;
    command_line_value channel_type_;

    task_manager task_manager_;
    mt_service mt_service_;
    network_service network_service_;

    expected<std::string> login(const service_provider * sp);
    expected<void> logout(const service_provider * sp, const std::string & session);

    expected<void> upload_file(const service_provider * sp, const std::string & session);

    expected<void> channel_list(const service_provider * sp, const std::string & session);
    expected<void> channel_create(const service_provider * sp, const std::string & session);

    async_task<expected<void>> channel_list_out(const std::string & server, http_message response);
  };
}

#endif // __VDS_CMD_VDS_CMD_APP_H_
