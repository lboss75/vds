/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "tcp_network_socket.h"
#include "network_service.h"
#include "tcp_socket_server.h"

TEST(http_tests, test_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(network_service);

    {
        auto sp = registrator.build("test_server");
        registrator.start(sp);

        //Start server
        vds::http_router router;
        router.add_static(
          "/",
          "<html><body>Hello World</body></html>");
       
        vds::tcp_socket_server server;
        server.start(
          sp,
          "127.0.0.1",
          8000,
          [&router](const vds::service_provider & sp, const vds::tcp_network_socket & s){
            vds::dataflow(
              vds::read_tcp_network_socket(s),
              vds::http_parser(),
              vds::http_middleware<vds::http_router>(router),
              vds::http_response_serializer(),
              vds::write_tcp_network_socket(s)
            )
            (
              [this](const vds::service_provider & sp) { },
              [this](const vds::service_provider & sp, std::exception_ptr ex) { },
              sp
            );
          })
        .wait(
          [](const vds::service_provider & sp) {
            std::cout << "Server has been closed\n";
          },
          [](const vds::service_provider & sp, std::exception_ptr ex) {
            FAIL() << vds::exception_what(ex);
          },
           sp
        );
        
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

TEST(http_tests, test_https_server)
{
  vds::service_registrator registrator;

  vds::network_service network_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(network_service);

  {
    auto sp = registrator.build("test_https_server");

    //Start server
    vds::http_router router;
    router.add_static(
      "/",
      "<html><body>Hello World</body></html>");

    std::function<void(int)> f;

    auto server_done_handler = vds::lambda_handler(
      [](const vds::service_provider & sp) {
      std::cout << "Server has been closed\n";
    }
    );
    auto server_error_handler = vds::lambda_handler(
      [](const vds::service_provider & sp, std::exception_ptr ex) {
      FAIL() << vds::exception_what(ex);
    }
    );

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


int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


