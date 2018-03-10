#ifndef __VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_
#define __VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_message.h"

namespace vds {

  class _file_upload_task  : public std::enable_shared_from_this<_file_upload_task> {
  public:
    _file_upload_task(const guid & channel_id)
        : channel_id_(channel_id){
    }

    async_task<> read_part(
        const service_provider & sp,
        const http_message& part);

    async_task<http_message> get_response(const vds::service_provider& sp);

  private:
    vds::guid channel_id_;
    uint8_t buffer_[1024];

    std::mutex result_mutex_;
    http_message result_;
    async_result<vds::http_message> result_done_;

    async_task<> read_file(const std::string & name, const vds::http_message& part);
  };
}

#endif //__VDS_WEB_SERVER_FILE_UPLOAD_TASK_P_H_
