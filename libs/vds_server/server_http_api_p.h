#ifndef __VDS_SERVER_SERVER_HTTP_API_P_H_
#define __VDS_SERVER_SERVER_HTTP_API_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _server_http_api
  {
  public:
    _server_http_api(const service_provider & sp);

    void start(const std::string & address, int port);

  private:
    service_provider sp_;

    std::unique_ptr<http_router> router_;
    certificate certificate_;
    asymmetric_private_key private_key_;


    class socket_session
    {
    public:
      socket_session(
        const http_router & router,
        const certificate & certificate,
        const asymmetric_private_key & private_key
      );

      class handler
      {
      public:
        handler(
          const socket_session & owner,
          const service_provider & sp,
          network_socket & s);

        void start();

      private:
        const service_provider & sp_;
        network_socket s_;
        ssl_tunnel tunnel_;
        const certificate & certificate_;
        const asymmetric_private_key & private_key_;
        server_logic server_logic_;
        delete_this<handler> done_handler_;

        std::function<void(std::exception *)> error_handler_;

        std::function<void(void)> http_server_done_;
        std::function<void(std::exception *)> http_server_error_;
      };
    private:
      const http_router & router_;
      const certificate & certificate_;
      const asymmetric_private_key & private_key_;

    };
  };
}

#endif // __VDS_SERVER_SERVER_HTTP_API_P_H_
