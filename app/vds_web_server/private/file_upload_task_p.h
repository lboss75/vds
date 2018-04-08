#ifndef __VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_
#define __VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_message.h"

namespace vds {
  class user_manager;

  class _file_upload_task  : public std::enable_shared_from_this<_file_upload_task> {
  public:
    _file_upload_task(const std::shared_ptr<user_manager> & user_mng)
    : user_mng_(user_mng){
    }

    async_task<> read_string_body(
      const std::shared_ptr<std::string>& buffer,
      const http_message& part);

    async_task<> read_part(
        const service_provider & sp,
        const http_message& part);

    async_task<http_message> get_response(const vds::service_provider& sp);

  private:
    std::shared_ptr<user_manager> user_mng_;
    const_data_buffer channel_id_;
    uint8_t buffer_[1024];

    std::mutex result_mutex_;
    http_message result_;
    async_result<vds::http_message> result_done_;

    async_task<> skip_part(const vds::http_message& part);
  };
}

#endif //__VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_
