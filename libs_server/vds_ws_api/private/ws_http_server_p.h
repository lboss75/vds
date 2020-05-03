#ifndef __VDS_WEB_SERVER_WEB_SERVER_P_H_
#define __VDS_WEB_SERVER_WEB_SERVER_P_H_
#include "tcp_socket_server.h"
#include "http_router.h"
#include "http_serializer.h"
#include "http_pipeline.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _ws_http_server : public std::enable_shared_from_this<_ws_http_server>
  {
  public:
    _ws_http_server(const service_provider * sp);
    ~_ws_http_server();

    vds::async_task<vds::expected<void>> start(uint16_t port);
    vds::async_task<vds::expected<void>> prepare_to_stop();
    
    //void add_auth_session(
    //    const std::string & id,
    //    const std::shared_ptr<auth_session> & session);

    //std::shared_ptr<auth_session> get_session(
    //  
    //  const std::string & session_id) const;

    //void kill_session(const std::string& session_id);

    //async_task<expected<std::tuple<bool, std::shared_ptr<stream_output_async<uint8_t>>>>> not_found_handler(
    //  const std::shared_ptr<http_async_serializer> & output_stream,
    //  const http_message& request);

  private:
    const service_provider * sp_;
    tcp_socket_server server_;
    http_router router_;

    //mutable std::shared_mutex auth_session_mutex_;
    //std::map<std::string, std::shared_ptr<auth_session>> auth_sessions_;
    //timer update_timer_;

    //std::shared_ptr<user_manager> www_user_mng_;

    //std::shared_ptr<user_manager> get_secured_context(
    //  const std::string & session_id) const;

    struct session_data : public std::enable_shared_from_this<session_data> {
      std::shared_ptr<vds::stream_output_async<uint8_t>> target_;
      std::shared_ptr<vds::http_async_serializer> stream_;
      std::shared_ptr<vds::http_pipeline> handler_;

      session_data(
        const service_provider * sp,
        const std::shared_ptr<vds::stream_output_async<uint8_t>> & target)
        : target_(target),
          stream_(std::make_shared<vds::http_async_serializer>(sp, target)) {
      }
    };

    async_task<expected<std::shared_ptr<stream_output_async<uint8_t>>>> process_message(
      const std::shared_ptr<http_async_serializer> & output_stream,
      const std::shared_ptr<session_data> & session,
      vds::http_message request);
  };
}

#endif // __VDS_WEB_SERVER_WEB_SERVER_P_H_
