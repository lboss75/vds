#ifndef __VDS_WEB_SERVER_WEB_SERVER_P_H_
#define __VDS_WEB_SERVER_WEB_SERVER_P_H_
#include "tcp_socket_server.h"
#include "http_middleware.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_message;
  class auth_session;
  class user_manager;

  class _web_server : public std::enable_shared_from_this<_web_server>
  {
  public:
    _web_server(const service_provider & sp);
    ~_web_server();

    void start(const service_provider& sp, const std::string & root_folder, uint16_t port);
    async_task<> prepare_to_stop(const service_provider &sp);

    async_task<http_message> route(
      const service_provider & sp,
      const http_message & request);

    void add_auth_session(
        const std::string & id,
        const std::shared_ptr<auth_session> & session);

  private:
    tcp_socket_server server_;
    http_middleware<_web_server> middleware_;
    http_router router_;

    mutable std::shared_mutex auth_session_mutex_;
    std::map<std::string, std::shared_ptr<auth_session>> auth_sessions_;

    void load_web(const std::string& path, const foldername & folder);

    std::shared_ptr<user_manager> get_secured_context(
        const service_provider & sp,
        const http_message &message) const;

    std::shared_ptr<auth_session> get_session(
      const service_provider & sp,
      const http_message &message) const;
  };
}

#endif // __VDS_WEB_SERVER_WEB_SERVER_P_H_
