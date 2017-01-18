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
      vds::network_socket && s)
      : error_handler_(this, error_method),
      done_handler_(this),
      router_(owner.router_),
      s_(s)
    {
    }

    void start()
    {
      vds::pipeline(
        vds::input_network_stream(this->s_),
        vds::http_parser(),
        vds::http_middleware(this->router_),
        vds::http_response_stream(),
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

    std::exception * error = nullptr;

    {
        auto sp = registrator.build();

        //Start server
        vds::http_router router;
        router.add_static(
          "/",
          "<html><body>Hello World</body></html>");

        vds::pipeline(
          vds::socket_server(sp, "127.0.0.1", 8000),
          vds::for_each<vds::network_socket &&>::create_handler(test_http_pipeline(router))
        )
        (
          []() {},
          [](std::exception * ex) {
            FAIL() << ex->what();
            delete ex;
          }
        );
        
        //Start client
        vds::http_request request("GET", "/");

        vds::barrier done;
        vds::sequence(
          vds::socket_connect(sp),
          vds::http_send_request(request)
        )
        ([&done](const vds::http_response & response) {
          ASSERT_EQ(response.body(), "<html><body>Hello World</body></html>");
          done.set();
        },
        [&done](std::exception * ex) {
          FAIL() << ex->what();
          delete ex;
          done.set();
        },
        "127.0.0.1",
        8000);

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


