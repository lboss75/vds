/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_network_socket.h"
#include "network_service.h"
#include "tcp_socket_server.h"
#include "logger.h"
#include "http_router.h"
#include "http_parser.h"
#include "http_middleware.h"
#include "http_serializer.h"
#include "http_request.h"
#include "barrier.h"
#include "async_stream.h"
#include "const_data_buffer.h"
#include "file.h"
#include "asymmetriccrypto.h"
#include "ssl_tunnel.h"
#include "crypto_service.h"

TEST(http_tests, test_https_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(network_service);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_https_server");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);

  vds::asymmetric_private_key pkey(vds::asymmetric_crypto::rsa4096());
  pkey.generate();

  vds::asymmetric_public_key public_key(pkey);

  vds::certificate::create_options opt;
  opt.country = "RU";
  opt.organization = "IVySoft";
  opt.name = "test certificate";

  vds::certificate cert = vds::certificate::create_new(public_key, pkey, opt);

  //Start server
  vds::http_router router;
  router.add_static(
    "/",
    "<html><body>Hello World</body></html>");

  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&router, &cert, &pkey](const vds::service_provider & sp, const vds::tcp_network_socket & s) {

    auto crypto_tunnel = std::make_shared<vds::ssl_tunnel>(false, &cert, &pkey);

    auto stream = std::make_shared<vds::continuous_stream<std::shared_ptr<vds::http_message>>>();
    vds::cancellation_token_source cancellation;
    vds::async_series(
      vds::create_async_task(
        [s, crypto_tunnel, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read<vds::continuous_stream<uint8_t>>(s.incoming()),
            vds::stream_write<vds::continuous_stream<uint8_t>>(crypto_tunnel->crypted_input())
          )(
            [done](const vds::service_provider & sp) {
              sp.get<vds::logger>()->debug(sp, "Server SSL Input closed");
              done(sp);
            },
            on_error, sp.create_scope("Server SSL Input"));
      }),
      vds::create_async_task(
        [s, crypto_tunnel, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
        vds::dataflow(
          vds::stream_read<vds::async_stream<uint8_t>>(crypto_tunnel->crypted_output()),
          vds::stream_write<vds::continuous_stream<uint8_t>>(s.outgoing())
          )(
            [done](const vds::service_provider & sp) {
              sp.get<vds::logger>()->debug(sp, "Server SSL Output closed");
              done(sp);
            },
            on_error, sp.create_scope("Server SSL Output"));
      }),
      vds::create_async_task(
        [s, stream, &router, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::async_stream<uint8_t>>(crypto_tunnel->decrypted_output()),
        vds::http_parser(
          [stream, &router, on_error](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) {
            vds::http_middleware<vds::http_router>(router).process(sp, request).wait(
              [stream](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & response) {
              stream->write_value_async(sp, response)
                .wait(
                  [stream](const vds::service_provider & sp) {
                    stream->write_async(sp, nullptr, 0).wait(
                      [](const vds::service_provider & sp, size_t) {
                      },
                      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                        sp.unhandled_exception(ex);
                      },
                      sp);
                  },
                  [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                    sp.unhandled_exception(ex);
                  },
                  sp);
            },
              on_error,
              sp);
      }
        )
      )(done, on_error, sp);
    }),
      vds::create_async_task(
        [s, stream, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<std::shared_ptr<vds::http_message>>>(stream),
        vds::http_serializer(),
        vds::stream_write<vds::continuous_stream<uint8_t>>(crypto_tunnel->decrypted_input())
      )(done, on_error, sp);
    })
      )
      .wait(
        [crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      FAIL() << ex->what();

    },
      sp);
    crypto_tunnel->start(sp);
  }).wait(
    [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Server has been started");
    b.set();
  },
    [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    FAIL() << ex->what();
    b.set();
  },
    sp
    );
  b.wait();
  b.reset();

  std::shared_ptr<vds::http_message> response;

  vds::asymmetric_private_key client_pkey(vds::asymmetric_crypto::rsa4096());
  client_pkey.generate();

  vds::asymmetric_public_key client_public_key(client_pkey);

  vds::certificate::create_options client_opt;
  client_opt.country = "RU";
  client_opt.organization = "IVySoft";
  client_opt.name = "test client certificate";

  vds::certificate client_cert = vds::certificate::create_new(client_public_key, client_pkey, client_opt);

  std::string answer;
  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &response, &answer, &client_cert, &client_pkey](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug(sp, "Connected");
    auto client_crypto_tunnel = std::make_shared<vds::ssl_tunnel>(true, &client_cert, &client_pkey);

    std::shared_ptr<vds::http_message> requests[] =
    {
      vds::http_request("GET", "/").get_message()
    };

    requests[0]->body()->write_all_async(sp, nullptr, 0).wait(
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {},
      sp);

    vds::cancellation_token_source cancellation;
    vds::async_series(
      vds::create_async_task(
        [s, client_crypto_tunnel, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read<vds::continuous_stream<uint8_t>>(s.incoming()),
            vds::stream_write<vds::continuous_stream<uint8_t>>(client_crypto_tunnel->crypted_input())
          )([done](const vds::service_provider & sp) {
            sp.get<vds::logger>()->debug(sp, "Client crypted input closed");
            done(sp);
          },
            [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<vds::logger>()->debug(sp, "Client crypted input error");
            on_error(sp, ex);
          },
            sp.create_scope("Client SSL Input"));
        }),
        vds::create_async_task(
          [s, client_crypto_tunnel, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read<vds::async_stream<uint8_t>>(client_crypto_tunnel->crypted_output()),
            vds::stream_write<vds::continuous_stream<uint8_t>>(s.outgoing())
          )([done](const vds::service_provider & sp) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output closed");
            done(sp);
          },
            [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output error");
            on_error(sp, ex);
          }, sp.create_scope("Client SSL Output"));
        }),
        vds::create_async_task(
          [s, &requests, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::dataflow_arguments<std::shared_ptr<vds::http_message>>(requests, 1),
            vds::http_serializer(),
            vds::stream_write<vds::continuous_stream<uint8_t>>(client_crypto_tunnel->decrypted_input())
          )(
            [done](const vds::service_provider & sp) {
            sp.get<vds::logger>()->debug(sp, "Client writer closed");
            done(sp);
          },
            [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<vds::logger>()->debug(sp, "Client writer error");
            on_error(sp, ex);
          },
            sp.create_scope("Client writer"));

        }),
        vds::create_async_task(
          [s, &response, &answer, client_crypto_tunnel, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read<vds::async_stream<uint8_t>>(client_crypto_tunnel->decrypted_output()),
            vds::http_parser(
              [&response, &answer, s, done, on_error, cancellation](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) {
            response = request;
            auto data = std::make_shared<std::vector<uint8_t>>();
            vds::dataflow(
              vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
              vds::collect_data(*data)
            )(
              [data, &answer, s, done, cancellation](const vds::service_provider & sp) {
              answer = std::string((const char *)data->data(), data->size());
              cancellation.cancel();
              done(sp);
            },
              [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              on_error(sp, ex);
            },
              sp.create_scope("Client read dataflow"));
      })
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        done(sp);
      },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader error");
        on_error(sp, ex);
      },
        sp.create_scope("Client reader"));
    })
      ).wait(
        [done, client_crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client closed");
      done(sp);
    },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug(sp, "Client error");
      on_error(sp, ex);
    },
      sp.create_scope("Client dataflow"));
    client_crypto_tunnel->start(sp);
  })
    .wait(
      [&b](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Request sent");
        b.set();
      },
      [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug(sp, "Request error");
        b.set();
      },
      sp.create_scope("Client"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}



