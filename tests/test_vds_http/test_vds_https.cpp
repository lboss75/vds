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
#include "http_client.h"
#include "http_server.h"
#include "random_buffer.h"
#include "encoding.h"

TEST(DISABLED_http_tests, test_https_server)
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
  vds::http_server http_server;
  vds::http_middleware<vds::http_router> middleware(router);

  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&middleware, &http_server, &cert, &pkey](const vds::service_provider & sp, const vds::tcp_network_socket & s) {

    auto crypto_tunnel = std::make_shared<vds::ssl_tunnel>(false, &cert, &pkey);

    vds::async_series(
      http_server.start(sp,
        crypto_tunnel->decrypted_output(), crypto_tunnel->decrypted_input(),
        [&middleware](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

          return middleware.process(sp, request);
      }),
      vds::create_async_task(
        [s, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
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
        [s, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
        vds::dataflow(
          vds::stream_read(crypto_tunnel->crypted_output()),
          vds::stream_write(s.outgoing())
          )(
            [done](const vds::service_provider & sp) {
              sp.get<vds::logger>()->debug(sp, "Server SSL Output closed");
              done(sp);
            },
            on_error, sp.create_scope("Server SSL Output"));
      })
      )
      .wait(
        [crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
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
  vds::http_client client;
  
  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &response, &answer, &client_cert, &client_pkey, &client](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug(sp, "Connected");
    auto client_crypto_tunnel = std::make_shared<vds::ssl_tunnel>(true, &client_cert, &client_pkey);

    vds::async_series(
      client.start(
        sp,
        client_crypto_tunnel->decrypted_output(), client_crypto_tunnel->decrypted_input(),
        [&response, &answer](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

          if (!request) {
            return vds::async_task<>::empty();
          }

          response = request;

          return vds::create_async_task(
            [&response, &answer](
              const std::function<void(const vds::service_provider & sp)> & task_done,
              const vds::error_handler & on_error,
              const vds::service_provider & sp)
          {
            auto data = std::make_shared<std::vector<uint8_t>>();
            vds::dataflow(
              vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
              vds::collect_data(*data)
            )(
              [data, &answer, task_done](const vds::service_provider & sp) {
              answer = std::string((const char *)data->data(), data->size());
              task_done(sp);
            },
              on_error,
              sp.create_scope("Client read dataflow"));

          });
      }),
      vds::create_async_task(
        [s, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
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
          [s, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read(client_crypto_tunnel->crypted_output()),
            vds::stream_write(s.outgoing())
          )([done](const vds::service_provider & sp) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output closed");
            done(sp);
          },
            [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output error");
            on_error(sp, ex);
          }, sp.create_scope("Client SSL Output"));
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

  std::shared_ptr<vds::http_message> request = vds::http_request("GET", "/").get_message();
  request->body()->write_all_async(sp, nullptr, 0).wait(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {},
    sp);


  client.send(sp, request)
  .then(
    [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
    })
  .wait(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
  sp);
  
  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}

static void copy_body(
  const vds::service_provider & sp,
  const std::shared_ptr<std::vector<uint8_t>> & buffer,
  const std::shared_ptr<vds::continuous_stream<uint8_t>> & source,
  const std::shared_ptr<vds::continuous_stream<uint8_t>> & dest)
{
  source->read_async(sp, buffer->data(), buffer->size())
  .wait([buffer, source, dest](const vds::service_provider & sp, size_t readed){
    if(0 == readed){
      dest->write_all_async(sp, nullptr, 0)
      .wait(
        [](const vds::service_provider & sp){
        },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & error){
          sp.unhandled_exception(error);
        },
        sp);
    }
    else {
      dest->write_all_async(sp, buffer->data(), readed)
      .wait(
        [buffer, source, dest](const vds::service_provider & sp){
          copy_body(sp, buffer, source, dest);
        },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & error){
          sp.unhandled_exception(error);
        },
        sp);
    }
  },
  [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & error){
    sp.unhandled_exception(error);
  },
  sp);
}

TEST(DISABLED_http_tests, test_ssl_streams)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
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
  vds::http_server http_server;

  auto server2client = std::make_shared<vds::continuous_stream<uint8_t>>();
  auto client2server = std::make_shared<vds::continuous_stream<uint8_t>>();

  vds::barrier b;
  //Server
    auto crypto_tunnel = std::make_shared<vds::ssl_tunnel>(false, &cert, &pkey);

    vds::async_series(
      http_server.start(sp,
        crypto_tunnel->decrypted_output(), crypto_tunnel->decrypted_input(),
        [](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

      return vds::create_async_task(
        [request](const std::function<void(const vds::service_provider & sp, std::shared_ptr<vds::http_message>)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp) {
        if (!request) {
          done(sp, std::shared_ptr<vds::http_message>());
        }
        else {
          std::string value;
          request->get_header("Content-Length", value);

          vds::http_response response(vds::http_response::HTTP_OK, "OK");
          response.add_header("Content-Length", value);
          auto result = response.create_message();

          copy_body(
            sp,
            std::make_shared<std::vector<uint8_t>>(1024),
            request->body(),
            result->body());

          done(sp, result);
        }
      });
    }),
      vds::create_async_task(
        [crypto_tunnel, client2server](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<uint8_t>>(client2server),
        vds::stream_write<vds::continuous_stream<uint8_t>>(crypto_tunnel->crypted_input())
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Server SSL Input closed");
        done(sp);
      },
        on_error, sp.create_scope("Server SSL Input"));
    }),
      vds::create_async_task(
        [crypto_tunnel, server2client](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read(crypto_tunnel->crypted_output()),
        vds::stream_write(server2client)
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Server SSL Output closed");
        done(sp);
      },
        on_error, sp.create_scope("Server SSL Output"));
    })
      )
      .wait(
        [crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
      sp);
    crypto_tunnel->start(sp);

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
  vds::http_client client;

    auto client_crypto_tunnel = std::make_shared<vds::ssl_tunnel>(true, &client_cert, &client_pkey);

    vds::async_series(
      client.start(
        sp,
        client_crypto_tunnel->decrypted_output(), client_crypto_tunnel->decrypted_input(),
        [&response, &answer](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

      if (!request) {
        return vds::async_task<>::empty();
      }

      response = request;

      return vds::create_async_task(
        [&response, &answer](
          const std::function<void(const vds::service_provider & sp)> & task_done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp)
      {
        auto data = std::make_shared<std::vector<uint8_t>>();
        vds::dataflow(
          vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
          vds::collect_data(*data)
        )(
          [data, &answer, task_done](const vds::service_provider & sp) {
          answer = std::string((const char *)data->data(), data->size());
          task_done(sp);
        },
          on_error,
          sp.create_scope("Client read dataflow"));

      });
    }),
      vds::create_async_task(
        [server2client, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<uint8_t>>(server2client),
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
        [client2server, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read(client_crypto_tunnel->crypted_output()),
        vds::stream_write(client2server)
      )([done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client crypted output closed");
        done(sp);
      },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug(sp, "Client crypted output error");
        on_error(sp, ex);
      }, sp.create_scope("Client SSL Output"));
    })
      ).wait(
        [&b, client_crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client closed");
      b.set();
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug(sp, "Client error");
      sp.unhandled_exception(ex);
    },
      sp.create_scope("Client dataflow"));
    client_crypto_tunnel->start(sp);

  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  client.send(sp, request)
    .then(
      [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
  })
    .wait(
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    sp.unhandled_exception(ex);
  },
      sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);

}

TEST(DISABLED_http_tests, test_streams)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_https_server");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  //Start server
  vds::http_server http_server;

  auto server2client = std::make_shared<vds::continuous_stream<uint8_t>>();
  auto client2server = std::make_shared<vds::continuous_stream<uint8_t>>();

  vds::barrier b;
  //Server
    http_server.start(sp,
      server2client, client2server,
      [](
        const vds::service_provider & sp,
        const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

    return vds::create_async_task(
      [request](const std::function<void(const vds::service_provider & sp, std::shared_ptr<vds::http_message>)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp) {
      if (!request) {
        done(sp, std::shared_ptr<vds::http_message>());
      }
      else {
        std::string value;
        request->get_header("Content-Length", value);

        vds::http_response response(vds::http_response::HTTP_OK, "OK");
        response.add_header("Content-Length", value);
        auto result = response.create_message();

        copy_body(
          sp,
          std::make_shared<std::vector<uint8_t>>(1024),
          request->body(),
          result->body());

        done(sp, result);
      }
    });
  })
    
    .wait(
      [](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Connection closed");
      },
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.unhandled_exception(ex);
      },
      sp);

  std::shared_ptr<vds::http_message> response;

  std::string answer;
  vds::http_client client;

    client.start(
      sp,
      client2server, server2client,
      [&response, &answer](
        const vds::service_provider & sp,
        const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

    if (!request) {
      return vds::async_task<>::empty();
    }

    response = request;

    return vds::create_async_task(
      [&response, &answer](
        const std::function<void(const vds::service_provider & sp)> & task_done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp)
    {
      auto data = std::make_shared<std::vector<uint8_t>>();
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
        vds::collect_data(*data)
      )(
        [data, &answer, task_done](const vds::service_provider & sp) {
        answer = std::string((const char *)data->data(), data->size());
        task_done(sp);
      },
        on_error,
        sp.create_scope("Client read dataflow"));

    });
  }).wait(
      [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Client closed");
    b.set();
  },
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    sp.get<vds::logger>()->debug(sp, "Client error");
    sp.unhandled_exception(ex);
  },
    sp.create_scope("Client dataflow"));

  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  client.send(sp, request)
    .then(
      [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
  })
    .wait(
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    sp.unhandled_exception(ex);
  },
      sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);

}

TEST(http_tests, test_http_serializer)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_http_serializer");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  vds::barrier b;

  std::vector<uint8_t> buffer;
  vds::dataflow(
    vds::dataflow_arguments<std::shared_ptr<vds::http_message>>(&request, 1),
    vds::http_serializer(),
    vds::collect_data(buffer)
  )(
    [&b](const vds::service_provider & sp) {
      b.set();
    },
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
    sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  std::string answer((const char *)buffer.data(), buffer.size());
  std::string expected = "GET / HTTP/1.0\nContent-Type: text/html; charset=utf-8\nContent-Length:" + std::to_string(test_data.length()) + "\n\n" + test_data;

  ASSERT_EQ(answer, expected);
}

TEST(http_tests, test_http_parser)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_http_parser");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  std::string answer;
  vds::barrier b;

  vds::dataflow(
    vds::dataflow_arguments<std::shared_ptr<vds::http_message>>(&request, 1),
    vds::http_serializer(),
    //random_filter(),
    vds::http_parser(
      [&answer, &b](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {
        if (!request) {
          return vds::async_task<>::empty();
        }

        return vds::create_async_task(
          [&b, &answer, request](
            const std::function<void(const vds::service_provider & sp)> & task_done,
            const vds::error_handler & on_error,
            const vds::service_provider & sp)
        {
          auto data = std::make_shared<std::vector<uint8_t>>();
          vds::dataflow(
            vds::stream_read<vds::continuous_stream<uint8_t>>(request->body()),
            vds::collect_data(*data)
          )(
            [data, &b, &answer, task_done](const vds::service_provider & sp) {
            b.set();
            answer = std::string((const char *)data->data(), data->size());
            task_done(sp);
          },
            on_error,
            sp.create_scope("Client read dataflow"));

        });
      }
    )
  )(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { sp.unhandled_exception(ex); },
    sp);

  b.wait();
  //Wait
  registrator.shutdown(sp);

  std::string expected = "GET / HTTP/1.0\nContent-Type: text/html; charset=utf-8\nContent-Length:" + std::to_string(test_data.length()) + "\n\n" + test_data;

  ASSERT_EQ(answer, expected);
}

TEST(DISABLED_http_tests, test_https_stream)
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
  vds::http_server http_server;

  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&http_server, &cert, &pkey](const vds::service_provider & sp, const vds::tcp_network_socket & s) {

    auto crypto_tunnel = std::make_shared<vds::ssl_tunnel>(false, &cert, &pkey);

    vds::async_series(
      http_server.start(sp,
        crypto_tunnel->decrypted_output(), crypto_tunnel->decrypted_input(),
        [](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

          return vds::create_async_task(
            [request](const std::function<void(const vds::service_provider & sp, std::shared_ptr<vds::http_message>)> & done,
               const vds::error_handler & on_error,
               const vds::service_provider & sp){
             if(!request){
               done(sp, std::shared_ptr<vds::http_message>());
             }
             else {
              std::string value;
              request->get_header("Content-Length", value);
              
              vds::http_response response(vds::http_response::HTTP_OK, "OK");
              response.add_header("Content-Length", value);
              auto result = response.create_message();
              
              copy_body(
                sp,
                std::make_shared<std::vector<uint8_t>>(1024),
                request->body(),
                result->body());
              
              done(sp, result);
             }
            });
      }),
      vds::create_async_task(
        [s, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
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
        [s, crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
        vds::dataflow(
          vds::stream_read(crypto_tunnel->crypted_output()),
          vds::stream_write(s.outgoing())
          )(
            [done](const vds::service_provider & sp) {
              sp.get<vds::logger>()->debug(sp, "Server SSL Output closed");
              done(sp);
            },
            on_error, sp.create_scope("Server SSL Output"));
      })
      )
      .wait(
        [crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
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
  vds::http_client client;
  
  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &response, &answer, &client_cert, &client_pkey, &client](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug(sp, "Connected");
    auto client_crypto_tunnel = std::make_shared<vds::ssl_tunnel>(true, &client_cert, &client_pkey);

    vds::async_series(
      client.start(
        sp,
        client_crypto_tunnel->decrypted_output(), client_crypto_tunnel->decrypted_input(),
        [&response, &answer](
          const vds::service_provider & sp,
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

          if (!request) {
            return vds::async_task<>::empty();
          }

          response = request;

          return vds::create_async_task(
            [&response, &answer](
              const std::function<void(const vds::service_provider & sp)> & task_done,
              const vds::error_handler & on_error,
              const vds::service_provider & sp)
          {
            auto data = std::make_shared<std::vector<uint8_t>>();
            vds::dataflow(
              vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
              vds::collect_data(*data)
            )(
              [data, &answer, task_done](const vds::service_provider & sp) {
              answer = std::string((const char *)data->data(), data->size());
              task_done(sp);
            },
              on_error,
              sp.create_scope("Client read dataflow"));

          });
      }),
      vds::create_async_task(
        [s, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
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
          [s, client_crypto_tunnel](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
          vds::dataflow(
            vds::stream_read(client_crypto_tunnel->crypted_output()),
            vds::stream_write(s.outgoing())
          )([done](const vds::service_provider & sp) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output closed");
            done(sp);
          },
            [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            sp.get<vds::logger>()->debug(sp, "Client crypted output error");
            on_error(sp, ex);
          }, sp.create_scope("Client SSL Output"));
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

  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());
  
  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);

  client.send(sp, request)
  .then(
    [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
    })
  .wait(
    [](const vds::service_provider & sp) {},
    [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    },
  sp);
  
  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);

}

