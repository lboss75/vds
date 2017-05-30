#ifndef __VDS_CLIENT_CLIENT_CONNECTION_H_
#define __VDS_CLIENT_CLIENT_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "tcp_network_socket.h"

namespace vds {

  template <typename connection_handler_type>
  class client_connection
  {
  public:
    client_connection(
      connection_handler_type * handler,
      const std::string & address,
      int port,
      certificate * client_certificate,
      asymmetric_private_key * client_private_key
    )
    : handler_(handler),
      address_(address),
      port_(port),
      client_certificate_(client_certificate),
      client_private_key_(client_private_key),
      state_(NONE)
    {
    }

    ~client_connection()
    {

    }

    enum connection_state
    {
      NONE,
      CONNECTING,
      CONNECTED,
      CONNECT_ERROR
    };

    connection_state state() const
    {
      return this->state_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_start() const
    {
      return this->connection_start_;
    }

    const std::chrono::time_point<std::chrono::system_clock> & connection_end() const
    {
      return this->connection_end_;
    }
    
    void connect(const service_provider & sp);
   
  private:
    connection_handler_type * handler_;
    std::string address_;
    int port_;
    certificate * client_certificate_;
    asymmetric_private_key * client_private_key_;
    connection_state state_;

    std::chrono::time_point<std::chrono::system_clock> connection_start_;
    std::chrono::time_point<std::chrono::system_clock> connection_end_;
  };

  template<typename connection_handler_type>
  inline void client_connection<connection_handler_type>::connect(const service_provider & sp)
  {
    tcp_network_socket::connect(
      sp,
      this->address_,
      this->port_)
      .then(
        [this](
          const std::function<void(const service_provider & sp)> & done,
          const error_handler & on_error,
          const service_provider & sp,
          tcp_network_socket && s) {

      sp.get<logger>()->debug(sp, "Connected");
      auto client_crypto_tunnel = std::make_shared<ssl_tunnel>(true, &client_cert, &client_pkey);

      std::shared_ptr<http_message> requests[] =
      {
        http_request("GET", "/").get_message()
      };

      requests[0]->body()->write_all_async(sp, nullptr, 0).wait(
        [](const service_provider & sp) {},
        [](const service_provider & sp, std::exception_ptr ex) {},
        sp);

      async_series(
        create_async_task(
          [s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          read_tcp_network_socket(s),
          stream_write<uint8_t>(client_crypto_tunnel->crypted_input())
        )([done](const service_provider & sp) {
          sp.get<logger>()->debug(sp, "Client crypted input closed");
          done(sp);
        },
          [on_error](const service_provider & sp, std::exception_ptr ex) {
          sp.get<logger>()->debug(sp, "Client crypted input error");
          on_error(sp, ex);
        },
          sp.create_scope("Client SSL Input"));
      }),
        create_async_task(
          [s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          stream_read<uint8_t>(client_crypto_tunnel->crypted_output()),
          write_tcp_network_socket(s)
        )([done](const service_provider & sp) {
          sp.get<logger>()->debug(sp, "Client crypted output closed");
          done(sp);
        },
          [on_error](const service_provider & sp, std::exception_ptr ex) {
          sp.get<logger>()->debug(sp, "Client crypted output error");
          on_error(sp, ex);
        }, sp.create_scope("Client SSL Output"));
      }),
        create_async_task(
          [s, &requests, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          dataflow_arguments<std::shared_ptr<http_message>>(requests, 1),
          http_serializer(),
          stream_write<uint8_t>(client_crypto_tunnel->decrypted_input())
        )(
          [done](const service_provider & sp) {
          sp.get<logger>()->debug(sp, "Client writer closed");
          done(sp);
        },
          [on_error](const service_provider & sp, std::exception_ptr ex) {
          sp.get<logger>()->debug(sp, "Client writer error");
          on_error(sp, ex);
        },
          sp.create_scope("Client writer"));

      }),
        create_async_task(
          [s, client_crypto_tunnel](const std::function<void(const service_provider & sp)> & done, const error_handler & on_error, const service_provider & sp) {
        dataflow(
          stream_read<uint8_t>(client_crypto_tunnel->decrypted_output()),
          http_parser(
            [s, done, on_error](const service_provider & sp, const std::shared_ptr<http_message> & request) {
          response = request;
          auto data = std::make_shared<std::vector<uint8_t>>();
          dataflow(
            stream_read<uint8_t>(response->body()),
            collect_data(*data)
          )(
            [data, s, done](const service_provider & sp) {
            answer = std::string((const char *)data->data(), data->size());
            tcp_network_socket(s).close();
            done(sp);
          },
            [on_error](const service_provider & sp, std::exception_ptr ex) {
            on_error(sp, ex);
          },
            sp.create_scope("Client read dataflow"));
        })
        )(
          [done](const service_provider & sp) {
          sp.get<logger>()->debug(sp, "Client reader closed");
          done(sp);
        },
          [on_error](const service_provider & sp, std::exception_ptr ex) {
          sp.get<logger>()->debug(sp, "Client reader error");
          on_error(sp, ex);
        },
          sp.create_scope("Client reader"));
      })
        ).wait(
          [done, client_crypto_tunnel](const service_provider & sp) {
        sp.get<logger>()->debug(sp, "Client closed");
        done(sp);
      },
          [on_error](const service_provider & sp, std::exception_ptr ex) {
        sp.get<logger>()->debug(sp, "Client error");
        on_error(sp, ex);
      },
        sp.create_scope("Client dataflow"));
      client_crypto_tunnel->start(sp);
    })
      .wait(
        [&b](const service_provider & sp) {
      sp.get<logger>()->debug(sp, "Request sent");
      b.set();
    },
        [&b](const service_provider & sp, std::exception_ptr ex) {
      sp.get<logger>()->debug(sp, "Request error");
      b.set();
    },
      sp.create_scope("Client"));
  }
}

#endif // __VDS_CLIENT_CLIENT_CONNECTION_H_
