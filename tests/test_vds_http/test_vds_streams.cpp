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
#include "async_buffer.h"
#include "const_data_buffer.h"
#include "file.h"
#include "asymmetriccrypto.h"
#include "ssl_tunnel.h"
#include "crypto_service.h"
#include "http_client.h"
#include "http_server.h"
#include "random_buffer.h"
#include "encoding.h"
#include "test_config.h"
#include "task_manager.h"

static vds::async_task<> copy_body(
  const vds::service_provider & sp,
  const std::shared_ptr<std::vector<uint8_t>> & buffer,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & source,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & dest)
{
  return source->read_async(sp, buffer->data(), buffer->size())
  .then([sp, buffer, source, dest](size_t readed){
    if(0 == readed){
      return dest->write_async(sp, nullptr, 0);
    }
    else {
      return dest->write_async(sp, buffer->data(), readed)
      .then(
        [sp, buffer, source, dest](){
          return copy_body(sp, buffer, source, dest);
        });
    }
  });
}

static vds::async_task<std::string> read_answer(
  const vds::service_provider & sp,
  const std::shared_ptr<uint8_t[1024]> & buffer,
  const std::shared_ptr<vds::continuous_buffer<uint8_t>> & source,
  const std::string & result = std::string())
{
  return source->read_async(sp, *buffer, 1024)
    .then([sp, buffer, source, result](size_t readed) -> vds::async_task<std::string> {
    if (0 == readed) {
      return [result]() { return result; };
    }
    else {
      return read_answer(sp, buffer, source, result + std::string((const char *)*buffer, readed));
    }
  });
}

TEST(http_tests, test_streams)
{
  vds::service_registrator registrator;
  vds::mt_service mt_service;
  vds::task_manager task_manager;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(task_manager);
  registrator.add(crypto_service);

  auto sp = registrator.build("test_https_server");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);


  //Start server
  vds::http_server http_server;

  auto server2client = std::make_shared<vds::continuous_buffer<uint8_t>>();
  auto client2server = std::make_shared<vds::continuous_buffer<uint8_t>>();

  std::shared_ptr<vds::http_message> response;

  std::string answer;
  vds::http_client client;
  
  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());

  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);
  
  vds::barrier b;
  //Server
  vds::async_series(
    http_server.start(sp,
      server2client, client2server,
      [sp](const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

    return [sp, request]()-> vds::async_task<> {
      if (!request) {
        return []() { return std::shared_ptr<vds::http_message>(); };
      }
      else {
        std::string value;
        request->get_header("Content-Length", value);

        vds::http_response response(vds::http_response::HTTP_OK, "OK");
        response.add_header("Content-Length", value);
        auto result = response.create_message();

        return copy_body(
          sp,
          std::make_shared<std::vector<uint8_t>>(1024),
          request->body(),
          result->body())
          .then([result](){ return result;});
      }
    };
  }),
    client.start(
      sp,
      client2server, server2client,
      [sp, &response, &answer](
        const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

    if (!request) {
      return vds::async_task<>::empty();
    }

    response = request;

    return read_answer(
      sp,
      std::make_shared<uint8_t[1024]>(),
      response->body())
    .then(
      [&answer](const std::string & result){
          answer = result;
        });
  }),
  client.send(sp, request)
    .then(
      [&client](const vds::service_provider & sp) {
    return client.send(sp, std::shared_ptr<vds::http_message>());
  })
    
  ).wait(
      [sp, &b]() {
    sp.get<vds::logger>()->debug("network", sp, "Client closed");
    b.set();
  },
      [sp](const std::shared_ptr<std::exception> & ex) {
    sp.get<vds::logger>()->debug("network", sp, "Client error");
    sp.unhandled_exception(ex);
  });


  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);

}


TEST(http_tests, test_https_stream)
{
  vds::service_registrator registrator;
  vds::task_manager task_manager;
  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
  vds::crypto_service crypto_service;

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(task_manager);
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

    vds::ssl_tunnel crypto_tunnel(false, &cert, &pkey);

    vds::async_series(
      http_server.start(sp,
        crypto_tunnel.decrypted_output(), crypto_tunnel.decrypted_input(),
        [sp](const std::shared_ptr<vds::http_message> & request) -> vds::async_task<std::shared_ptr<vds::http_message>> {

          return [sp, request]() -> vds::async_task<std::shared_ptr<vds::http_message>> {
             if(!request){
               return []() { return std::shared_ptr<vds::http_message>(); };
             }
             else {
              std::string value;
              request->get_header("Content-Length", value);
              
              vds::http_response response(vds::http_response::HTTP_OK, "OK");
              response.add_header("Content-Length", value);
              auto result = response.create_message();
              
              return copy_body(
                sp,
                std::make_shared<std::vector<uint8_t>>(1024),
                request->body(),
                result->body()).then([result]() {
                return result;
              });
             }
            };
      }),
      vds::copy_stream(
        sp,
        s.incoming(),
        crypto_tunnel.crypted_input()
      ),
        vds::copy_stream(
          sp,
          crypto_tunnel.crypted_output(),
          s.outgoing()
        )
      ).wait(
        [sp, crypto_tunnel]() {
      sp.get<vds::logger>()->debug("SSL", sp, "Connection closed");
    },
        [sp](const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    });
    crypto_tunnel.start(sp);
  }).wait(
    [sp, &b]() {
      sp.get<vds::logger>()->debug("SSL", sp, "Server has been started");
      b.set();
    },
    [sp, &b](const std::shared_ptr<std::exception> & ex) {
      FAIL() << ex->what();
      b.set();
    });
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
      [sp, &b, &response, &answer, &client_cert, &client_pkey, &client](const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug("SSL", sp, "Connected");
    vds::ssl_tunnel client_crypto_tunnel(true, &client_cert, &client_pkey);

    client_crypto_tunnel.start(sp);

    return vds::async_series(
      client.start(
        sp,
        client_crypto_tunnel.decrypted_output(), client_crypto_tunnel.decrypted_input(),
        [sp, &response, &answer](
          const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

          if (!request) {
            return vds::async_task<>::empty();
          }

          response = request;

          return read_answer(
            sp,
            std::make_shared<uint8_t[1024]>(),
            response->body())
            .then(
              [&answer](static std::string & result) {
              answer = result;
            });
      }),
      vds::copy_stream(
        sp,
        s.incoming(),
        client_crypto_tunnel.crypted_input()
      ),
        vds::copy_stream(
          sp,
          client_crypto_tunnel.crypted_output(),
          s.outgoing()
      )
      );
  })
    .wait(
      [sp, &b]() {
        sp.get<vds::logger>()->debug("SSL", sp, "Request sent");
        b.set();
      },
      [sp](const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug("SSL", sp, "Request error");
      });

  random_buffer buf;
  auto test_data = vds::base64::from_bytes(buf.data(), buf.size());
  
  std::shared_ptr<vds::http_message> request = vds::http_request::simple_request(sp, "GET", "/", test_data);
    
  client.send(sp, request)
  .then(
    [sp, &client]() {
      return client.send(sp, std::shared_ptr<vds::http_message>());
    })
  .execute(
    [sp](const std::shared_ptr<std::exception> & ex) {
      sp.unhandled_exception(ex);
    });
  
  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, test_data);

}

TEST(http_tests, test_ssl_streams)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(
    test_config::instance().log_level(),
    test_config::instance().modules());
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

  auto server2client = std::make_shared<vds::continuous_buffer<uint8_t>>();
  auto client2server = std::make_shared<vds::continuous_buffer<uint8_t>>();

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
      vds::dataflow(
        vds::stream_read<vds::continuous_buffer<uint8_t>>(client2server),
        vds::stream_write<vds::continuous_buffer<uint8_t>>(crypto_tunnel->crypted_input())
      ),
      vds::dataflow(
        vds::stream_read(crypto_tunnel->crypted_output()),
        vds::stream_write(server2client)
      )
      )
      .wait(
        [crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug("SSL", sp, "Connection closed");
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
      auto data = std::make_shared<std::vector<uint8_t>>();

      return vds::dataflow(
          vds::stream_read<vds::continuous_buffer<uint8_t>>(response->body()),
          vds::collect_data(*data))
        .then(
          [data, &answer](
            const std::function<void(const vds::service_provider & sp)> & task_done,
            const vds::error_handler & on_error,
            const vds::service_provider & sp) {
          answer = std::string((const char *)data->data(), data->size());
          task_done(sp);
        });
    }),
      vds::dataflow(
        vds::stream_read<vds::continuous_buffer<uint8_t>>(server2client),
        vds::stream_write<vds::continuous_buffer<uint8_t>>(client_crypto_tunnel->crypted_input())
      ),
      vds::dataflow(
        vds::stream_read(client_crypto_tunnel->crypted_output()),
        vds::stream_write(client2server)
      )
      ).wait(
        [&b, client_crypto_tunnel](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug("test", sp, "Client closed");
      b.set();
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug("test", sp, "Client error");
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


