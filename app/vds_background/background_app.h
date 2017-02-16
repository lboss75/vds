#ifndef __VDS_BACKGROUND_BACKGROUND_APP_H_
#define __VDS_BACKGROUND_BACKGROUND_APP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class vrt_object;

  class background_app : public console_app<background_app>
  {
    using base_class = console_app<background_app>;
  public:
    background_app();

    void main(const service_provider & sp);
    
    void register_services(service_registrator & registrator);
    void register_command_line(command_line & cmd_line);

  private:
    command_line_set start_server_command_set_;

    network_service network_service_;
    crypto_service crypto_service_;
    
    std::unique_ptr<http_router> router_;
    certificate certificate_;
    asymmetric_private_key private_key_;

    task_manager task_manager_;
    storage_service storage_;
    client client_;
    server server_;
    
    std::function<void(void)> http_server_done_;
    std::function<void(std::exception *)> http_server_error_;

    void http_server_closed();
    void http_server_error(std::exception *);

    class socket_session
    {
    public:
      socket_session(
        const service_provider & sp,
        const http_router & router,
        const certificate & certificate,
        const asymmetric_private_key & private_key
      );

      class handler
      {
      public:
        handler(
          const socket_session & owner,
          network_socket & s);

        void start();

      private:
        const service_provider & sp_;
        network_socket s_;
        ssl_peer peer_;
        const certificate & certificate_;
        const asymmetric_private_key & private_key_;
        client client_;
        server_logic server_logic_;
        delete_this<handler> done_handler_;

        std::function<void(std::exception *)> error_handler_;

        std::function<void(void)> http_server_done_;
        std::function<void(std::exception *)> http_server_error_;
      };
    private:
      const service_provider & sp_;
      const http_router & router_;
      const certificate & certificate_;
      const asymmetric_private_key & private_key_;

    };
  };
}

#endif // __VDS_BACKGROUND_BACKGROUND_APP_H_
