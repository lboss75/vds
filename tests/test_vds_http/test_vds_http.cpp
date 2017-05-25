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

TEST(http_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);

  registrator.add(mt_service);
  registrator.add(file_logger);
  registrator.add(network_service);

  auto sp = registrator.build("test_server");
  registrator.start(sp);

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
    [&router](const vds::service_provider & sp, const vds::tcp_network_socket & s) {
    auto responses = new std::shared_ptr<vds::http_message>[1];
    auto stream = std::make_shared<vds::async_stream<std::shared_ptr<vds::http_message>>>();
    vds::async_series(
      vds::create_async_task(
        [s, stream, &router, responses](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::read_tcp_network_socket(s),
        vds::http_parser(
          [stream, &router, responses](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) {
        responses[0] = vds::http_middleware<vds::http_router>(router).process(sp, request);
        stream->write_all_async(sp, responses, 1)
          .wait(
            [](const vds::service_provider & sp) {
        },
            [](const vds::service_provider & sp, std::exception_ptr ex) {
        },
          sp);
      }
        )
      )(done, on_error, sp);
    }),
      vds::create_async_task(
        [s, stream](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<std::shared_ptr<vds::http_message>>(stream),
        vds::http_serializer(),
        vds::write_tcp_network_socket(s)
      )(done, on_error, sp);
    })
      )
      .wait(
        [responses](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
      delete responses;
    },
        [responses](const vds::service_provider & sp, std::exception_ptr ex) {
      delete responses;
      FAIL() << vds::exception_what(ex);

    },
      sp);
  }).wait(
      [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Server has been started");
    b.set();
  },
      [&b](const vds::service_provider & sp, std::exception_ptr ex) {
    FAIL() << vds::exception_what(ex);
    b.set();
  },
    sp
    );
  b.wait();
  b.reset();

  std::shared_ptr<vds::http_message> response;

  std::string answer;
  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &response, &answer](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        vds::tcp_network_socket && s) {

    sp.get<vds::logger>()->debug(sp, "Connected");

    std::shared_ptr<vds::http_message> requests[] =
    {
      vds::http_request("GET", "/").get_message()
    };

    requests[0]->body()->write_all_async(sp, nullptr, 0).wait(
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, std::exception_ptr ex) {},
      sp);

    vds::async_series(
      vds::create_async_task(
        [s, &requests](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::dataflow_arguments<std::shared_ptr<vds::http_message>>(requests, 1),
        vds::http_serializer(),
        vds::write_tcp_network_socket(s)
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client writer closed");
        done(sp);
      },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client writer error");
        on_error(sp, ex);
      },
        sp.create_scope("Client writer"));

    }),
      vds::create_async_task(
        [s, &response, &answer](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::read_tcp_network_socket(s),
        vds::http_parser(
          [&response, &answer, s, done, on_error](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) {
        response = request;
        auto data = std::make_shared<std::vector<uint8_t>>();
        vds::dataflow(
          vds::stream_read<uint8_t>(response->body()),
          vds::collect_data(*data)
        )(
          [data, &answer, s, done](const vds::service_provider & sp) {
          answer = std::string((const char *)data->data(), data->size());
          vds::tcp_network_socket(s).close();
          done(sp);
        },
          [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
          on_error(sp, ex);
        },
          sp.create_scope("Client read dataflow"));

      })
      )(
        [done](const vds::service_provider & sp) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        done(sp);
      },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader error");
        on_error(sp, ex);
      },
        sp.create_scope("Client reader"));
    })
      ).wait(
        [done](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client closed");
      done(sp);
    },
        [on_error](const vds::service_provider & sp, std::exception_ptr ex) {
      sp.get<vds::logger>()->debug(sp, "Client error");
      on_error(sp, ex);
    },
      sp.create_scope("Client dataflow"));
  })
  .wait(
    [&b](const vds::service_provider & sp) { sp.get<vds::logger>()->debug(sp, "Request sent"); b.set(); },
    [&b](const vds::service_provider & sp, std::exception_ptr ex) { sp.get<vds::logger>()->debug(sp, "Request error"); b.set(); },
    sp.create_scope("Client"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}
/*
TEST(http_tests, test_https_server)
{
  vds::service_registrator registrator;

  vds::network_service network_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(network_service);

  {
    auto sp = registrator.build("test_https_server");
    registrator.start(sp);

    //Start server
    vds::http_router router;
    router.add_static(
      "/",
      "<html><body>Hello World</body></html>");

    std::function<void(int)> f;

    test_http_pipeline pipeline(router);
    vds::dataflow(
      vds::socket_server(sp, "127.0.0.1", 8000),
      vds::create_socket_session([&pipeline](const vds::service_provider & sp, vds::network_socket & s){
        (new test_http_pipeline::handler(pipeline, s))->start(sp);
      })
    )
    (
      server_done_handler,
      server_error_handler,
      sp);

    //Start client
    vds::http_request request("GET", "/");
    vds::http_outgoing_stream outgoing_stream;

    vds::http_simple_response_reader response_reader;

    vds::barrier done;

    auto done_handler = vds::lambda_handler(
      [&done](const vds::service_provider & sp, const std::string & body) {
      ASSERT_EQ(body, "<html><body>Hello World</body></html>");
      done.set();
    }
    );

    auto error_handler = vds::lambda_handler(
      [&done](const vds::service_provider & sp, std::exception_ptr ex) {
      done.set();
      FAIL() << vds::exception_what(ex);
    }
    );
    vds::dataflow(
      vds::socket_connect(),
      vds::http_send_request<
      vds::http_simple_response_reader
      >(
        request,
        outgoing_stream,
        response_reader)
    )
    (
      done_handler,
      error_handler,
      sp,
      (const char *)"127.0.0.1",
      8000
      );

    //Wait
    done.wait();

    registrator.shutdown(sp);
  }

}
*/

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


