/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"

class test_http_pipeline
{
private:
  class error_method_type
  {
  public:
    void operator()(std::exception_ptr ex)
    {
      FAIL() << vds::exception_what(ex);
    }
  };

public:
  test_http_pipeline(
    const vds::http_router & router)
    : router_(router)
  {
  }

  ~test_http_pipeline()
  {

  }

  class handler
  {
  public:
    handler(
      const test_http_pipeline & owner,
      vds::network_socket & s)
      :
      s_(std::move(s)),
      router_(owner.router_),
      done_handler_(this),
      error_handler_(this, const_cast<test_http_pipeline &>(owner).error)
    {
    }

    void start(const vds::service_provider & sp)
    {
      std::cout << "New connection\n";
      
      vds::dataflow(
        vds::input_network_stream(sp, this->s_),
        vds::http_parser(sp),
        vds::http_middleware<vds::http_router>(this->router_),
        vds::http_response_serializer(),
        vds::output_network_stream(sp, this->s_)
      )
      (
        [this](const vds::service_provider & sp) { this->done_handler_(); },
        [this](const vds::service_provider & sp, std::exception_ptr ex) { this->error_handler_(ex); },
        sp
      );
    }
  private:
    vds::network_socket s_;
    const vds::http_router & router_;
    vds::delete_this<handler> done_handler_;
    
    vds::auto_cleaner<handler, error_method_type> error_handler_;
  };

private:
  const vds::http_router & router_;
  error_method_type error;
};

TEST(http_tests, test_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(network_service);

    {
        auto sp = registrator.build("test_server");

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
        
        vds::dataflow(
          vds::socket_server(sp, "127.0.0.1", 8000),
          vds::for_each<vds::network_socket &>::create_handler(test_http_pipeline(router))
        )
        (
          server_done_handler,
          server_error_handler,
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
          vds::socket_connect(sp),
          vds::http_send_request<
            vds::http_simple_response_reader
            >(
              sp,
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

    vds::dataflow(
      vds::socket_server(sp, "127.0.0.1", 8000),
      vds::for_each<const vds::service_provider &, vds::network_socket &>::create_handler(test_http_pipeline(router))
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
      vds::socket_connect(sp),
      vds::http_send_request<
      vds::http_simple_response_reader
      >(
        sp,
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


