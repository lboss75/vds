#ifndef __VDS_WEB_SERVER_API_CONTROLLER_H_
#define __VDS_WEB_SERVER_API_CONTROLLER_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "http_message.h"
#include "service_provider.h"
#include "web_server_p.h"
#include "user_channel.h"

namespace vds {
  class api_controller {
  public:
    static std::shared_ptr<json_value> get_channels(
        const service_provider& sp,
        user_manager & user_mng,
        const std::shared_ptr<_web_server> & owner,
        const http_message& message);

    static async_task<http_message> get_login_state(
      const service_provider& sp,
      const std::string & login,
      const std::string & password,
      const std::shared_ptr<_web_server>& owner,
      const http_message& message);

    static async_task <http_message> create_channel(
      const service_provider &sp,
      const std::shared_ptr<user_manager> &user_mng,
      const std::string & name);

    static async_task<std::shared_ptr<json_value>> channel_feed(
      const service_provider& sp,
      const std::shared_ptr<user_manager> & user_mng,
      const std::shared_ptr<_web_server>& owner,
      const const_data_buffer & channel_id);

    static async_task<
      std::string /*content_type*/,
      std::string /*filename*/,
      size_t /*body_size*/,
      std::shared_ptr<continuous_buffer<uint8_t>> /*output_stream*/>
    download_file(
      const service_provider& sp,
      const std::shared_ptr<user_manager>& user_mng,
      const std::shared_ptr<_web_server>& owner,
      const const_data_buffer& channel_id,
      const const_data_buffer& file_hash);

    static vds::async_task<std::shared_ptr<vds::json_value>>
    user_devices(const service_provider &sp, const std::shared_ptr<user_manager> &user_mng,
                             const std::shared_ptr<_web_server> &owner);

    static async_task<> lock_device(const vds::service_provider &sp, const std::shared_ptr<vds::user_manager> &user_mng,
                                        const std::shared_ptr<vds::_web_server> &owner, const std::string &device_name,
                                        const std::string &local_path, uint64_t reserved_size);

    static async_task<std::shared_ptr<vds::json_value>>
    offer_device(
        const vds::service_provider &sp,
        const std::shared_ptr<user_manager> &user_mng,
        const std::shared_ptr<_web_server> &owner);

    static async_task<std::shared_ptr<vds::json_value>>
    get_statistics(
      const service_provider& sp,
      const std::shared_ptr<_web_server>& owner,
      const http_message & message);

    static std::shared_ptr<json_value>
    get_invite(
      const service_provider& sp,
      user_manager& user_mng,
      const std::shared_ptr<_web_server>& owner,
      const http_message& message);

    static async_task<std::shared_ptr<vds::json_value>>
      get_register_requests(
        const service_provider& sp,
        const std::shared_ptr<_web_server>& owner);

    static async_task<std::shared_ptr<vds::json_value>>
      get_register_request(
        const service_provider& sp,
        const std::shared_ptr<_web_server>& owner,
        const const_data_buffer & request_id);

    static async_task<const_data_buffer>
      get_register_request_body(
        const service_provider& sp,
        const std::shared_ptr<_web_server>& owner,
        const const_data_buffer & request_id);

    static async_task<http_message> get_session(
      const service_provider& sp,
      const std::shared_ptr<_web_server>& owner,
      const std::string& session_id);
    
    static async_task<http_message> logout(
      const service_provider& sp,
      const std::shared_ptr<_web_server>& owner,
      const std::string& session_id);

  private:
    static std::shared_ptr<json_object> channel_serialize(const vds::user_channel & channel);

  };
}

#endif //__VDS_WEB_SERVER_API_CONTROLLER_H_
