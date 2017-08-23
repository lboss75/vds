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
#include "http_pipeline.h"

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
  vds::imt_service::enable_async(sp);

  //Start server
  vds::http_router router;
  router.add_static(
    "/",
    "<html><body>Hello World</body></html>");

  vds::http_middleware<vds::http_router> middleware(router);

  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&middleware](const vds::service_provider & sp, const vds::tcp_network_socket & s) {
    auto stream = std::make_shared<vds::continuous_stream<std::shared_ptr<vds::http_message>>>();
    vds::cancellation_token_source cancellation;
    vds::async_series(
      vds::create_async_task(
        [s, stream, &middleware, &cancellation](
          const std::function<void(const vds::service_provider & sp)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<uint8_t>>(s.incoming()),
        vds::http_parser(
          [stream, &middleware, on_error](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & request) -> vds::async_task<> {

          if (!request) {
            return stream->write_all_async(sp, nullptr, 0);
          }

        return middleware.process(sp, request)
          .then([stream](const vds::service_provider & sp, const std::shared_ptr<vds::http_message> & response) {
            return stream->write_value_async(sp, response);
          });
        })
      )(done, on_error, sp);
    }),
      vds::create_async_task(
        [s, stream, &cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read<vds::continuous_stream<std::shared_ptr<vds::http_message>>>(stream),
        vds::http_serializer(),
        vds::stream_write<vds::continuous_stream<uint8_t>>(s.outgoing())
      )(done, on_error, sp);
    })
      )
      .wait(
        [](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Connection closed");
    },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      FAIL() << ex->what();

    },
      sp);
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
  vds::cancellation_token_source cancellation;

  std::string answer;
  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &response, &answer, &cancellation](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::tcp_network_socket & s) {

    sp.get<vds::logger>()->debug(sp, "Connected");

    std::shared_ptr<vds::http_message> requests[] =
    {
      vds::http_request("GET", "/").get_message()
    };

    requests[0]->body()->write_all_async(sp, nullptr, 0).wait(
      [](const vds::service_provider & sp) {},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {},
      sp);
    
    auto input_commands = std::make_shared<vds::async_stream<std::shared_ptr<vds::http_message>>>();
    auto output_commands = std::make_shared<vds::async_stream<std::shared_ptr<vds::http_message>>>();
    vds::async_series(
      vds::http_pipeline(s.incoming(), input_commands, output_commands, s.outgoing()),
      vds::create_async_task(
        [s, &requests, &cancellation, output_commands](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::dataflow_arguments<std::shared_ptr<vds::http_message>>(requests, 1),
        vds::stream_write(output_commands)
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
        [s, &response, &answer, &cancellation, input_commands](
          const std::function<void(const vds::service_provider & sp)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp) {
      vds::dataflow(
        vds::stream_read(input_commands),
        vds::dataflow_consumer<std::shared_ptr<vds::http_message>>(
          [&response, &answer, s, done, on_error, &cancellation](
            const vds::service_provider & sp,
            const std::shared_ptr<vds::http_message> * requests,
            size_t count) -> vds::async_task<size_t> {
        return vds::create_async_task(
          [&response, &answer, s, done, on_error, &cancellation, requests, count](
            const std::function<void(const vds::service_provider & sp)> & task_done,
            const vds::error_handler & on_error,
            const vds::service_provider & sp)
        {
          if (0 == count) {
            cancellation.cancel();
            done(sp);

            task_done(sp);
          }
          else {
            response = request;
            auto data = std::make_shared<std::vector<uint8_t>>();
            vds::dataflow(
              vds::stream_read<vds::continuous_stream<uint8_t>>(response->body()),
              vds::collect_data(*data)
            )(
              [data, &answer, s, done, &cancellation](const vds::service_provider & sp) {
              answer = std::string((const char *)data->data(), data->size());
              cancellation.cancel();
              done(sp);
            },
              [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
              on_error(sp, ex);
            },
              sp.create_scope("Client read dataflow"));

          }
        });
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
        [done](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client closed");
      done(sp);
    },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug(sp, "Client error");
      on_error(sp, ex);
    },
      sp.create_scope("Client dataflow"));
  })
  .wait(
    [&b](const vds::service_provider & sp) { sp.get<vds::logger>()->debug(sp, "Request sent"); b.set(); },
    [&b](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { sp.get<vds::logger>()->debug(sp, "Request error"); b.set(); },
    sp.create_scope("Client"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  ASSERT_EQ(answer, "<html><body>Hello World</body></html>");

}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


