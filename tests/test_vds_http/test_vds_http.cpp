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
    void operator()(std::exception * ex)
    {
      FAIL() << ex->what();
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
      const vds::service_provider & sp,
      vds::network_socket & s)
      :
      sp_(sp),
      s_(std::move(s)),
      router_(owner.router_),
      done_handler_(this),
      error_handler_(this, const_cast<test_http_pipeline &>(owner).error)
    {
    }

    void start()
    {
      std::cout << "New connection\n";
      
      vds::dataflow(
        vds::input_network_stream(this->sp_, this->s_),
        vds::http_parser(this->sp_),
        vds::http_middleware<vds::http_router>(this->router_),
        vds::http_response_serializer(),
        vds::output_network_stream(this->sp_, this->s_)
      )
      (
        this->done_handler_,
        this->error_handler_
      );
    }
  private:
    vds::service_provider sp_;
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
        auto sp = registrator.build();

        //Start server
        vds::http_router router(sp);
        router.add_static(
          "/",
          "<html><body>Hello World</body></html>");

        std::function<void(int)> f;

        auto server_done_handler = vds::lambda_handler(
          []() {
            std::cout << "Server has been closed\n";
          }
        );
        auto server_error_handler = vds::lambda_handler(
          [](std::exception * ex) {
            std::unique_ptr<std::exception> ex_(ex);
            FAIL() << ex_->what();
          }
        );
        
        vds::dataflow(
          vds::socket_server(sp, "127.0.0.1", 8000),
          vds::for_each<const vds::service_provider &, vds::network_socket &>::create_handler(test_http_pipeline(router))
        )
        (
          server_done_handler,
          server_error_handler
        );
        
        //Start client
        vds::http_request request("GET", "/");
        vds::http_outgoing_stream outgoing_stream;
        
        vds::http_simple_response_reader response_reader;

        vds::barrier done;
        
        auto done_handler = vds::lambda_handler(
          [&done](const std::string & body) {
            ASSERT_EQ(body, "<html><body>Hello World</body></html>");
            done.set();
          }
        );
        
        auto error_handler = vds::lambda_handler(
          [&done](std::exception * ex) {
            std::unique_ptr<std::exception> ex_(ex);
            done.set();
            FAIL() << ex_->what();
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
         (const char *)"127.0.0.1",
         8000
        );

        //Wait
        done.wait();
    }

    registrator.shutdown();
}

TEST(http_tests, test_https_server)
{
  vds::service_registrator registrator;

  vds::network_service network_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(network_service);

  {
    auto sp = registrator.build();

    //Start server
    vds::http_router router(sp);
    router.add_static(
      "/",
      "<html><body>Hello World</body></html>");

    std::function<void(int)> f;

    auto server_done_handler = vds::lambda_handler(
      []() {
      std::cout << "Server has been closed\n";
    }
    );
    auto server_error_handler = vds::lambda_handler(
      [](std::exception * ex) {
      std::unique_ptr<std::exception> ex_(ex);
      FAIL() << ex_->what();
    }
    );

    vds::dataflow(
      vds::socket_server(sp, "127.0.0.1", 8000),
      vds::for_each<const vds::service_provider &, vds::network_socket &>::create_handler(test_http_pipeline(router))
    )
    (
      server_done_handler,
      server_error_handler
      );

    //Start client
    vds::http_request request("GET", "/");
    vds::http_outgoing_stream outgoing_stream;

    vds::http_simple_response_reader response_reader;

    vds::barrier done;

    auto done_handler = vds::lambda_handler(
      [&done](const std::string & body) {
      ASSERT_EQ(body, "<html><body>Hello World</body></html>");
      done.set();
    }
    );

    auto error_handler = vds::lambda_handler(
      [&done](std::exception * ex) {
      std::unique_ptr<std::exception> ex_(ex);
      done.set();
      FAIL() << ex_->what();
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
      (const char *)"127.0.0.1",
      8000
      );

    //Wait
    done.wait();
  }

  registrator.shutdown();
}


int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


