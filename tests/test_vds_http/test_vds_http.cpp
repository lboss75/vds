/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"

class test_http_pipeline
{
public:
  test_http_pipeline(
    const vds::http_router & router)
    : router_(router)
  {
  }

  ~test_http_pipeline()
  {

  }

  template <
    typename error_method_type
  >
  class handler
  {
  public:
    handler(
      error_method_type & error_method,
      const test_http_pipeline & owner,
      vds::network_socket & s)
      :      
      s_(std::move(s)),
      router_(owner.router_),
      done_handler_(this),
      error_handler_(this, error_method)
    {
    }

    void start()
    {
      std::cout << "New connection\n";
      
      vds::sequence(
        vds::input_network_stream(this->s_),
        vds::http_parser(),
        vds::http_middleware(this->router_),
        vds::http_response_serializer(),
        vds::output_network_stream(this->s_)
      )
      (
        this->done_handler_,
        this->error_handler_
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
};

TEST(http_tests, test_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::mt_service mt_service;
    vds::console_logger console_logger;

    registrator.add(console_logger);
    registrator.add(mt_service);
    registrator.add(network_service);

    {
        auto sp = registrator.build();

        //Start server
        vds::http_router router;
        router.add_static(
          "/",
          "<html><body>Hello World</body></html>");

        std::function<void(int)> f;

        vds::empty_handler empty_handler;
        auto server_error_handler = vds::lambda_handler(
          [](std::exception * ex) {
            FAIL() << ex->what();
            delete ex;
          }
        );
        vds::sequence(
          vds::socket_server(sp, "127.0.0.1", 8000),
          vds::for_each<vds::network_socket>::create_handler(test_http_pipeline(router))
        )
        (
          empty_handler,
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
            FAIL() << ex->what();
            delete ex;
            done.set();
          }
        );        
        vds::sequence(
          vds::socket_connect(sp),
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
         "127.0.0.1",
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


