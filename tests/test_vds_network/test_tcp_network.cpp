/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "tcp_network_socket.h"
#include "service_provider.h"
#include "mt_service.h"
#include "network_service.h"
#include "dataflow.h"
#include "logger.h"
#include "file.h"
#include "barrier.h"
#include "tcp_socket_server.h"
#include "random_buffer.h"
#include "random_reader.h"
#include "compare_data.h"

class read_for_newline
{
public:
  read_for_newline(
    const std::function<void(const vds::service_provider & sp, const std::string &)> & done_callback)
  : done_callback_(done_callback)
  {
  }

  using incoming_item_type = uint8_t;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_target<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_target<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const read_for_newline & args)
    : base_class(context),
      done_callback_(args.done_callback_)
    {
    }
  
    size_t sync_push_data(const vds::service_provider & sp)
    {
      if(0 == this->input_buffer_size()) {
        if (this->done_callback_) {
          std::function<void(const vds::service_provider & sp, const std::string &)> f;
          this->done_callback_.swap(f);
          f(sp, this->buffer_);
        }
      }
      else {
        this->buffer_.append(reinterpret_cast<const char *>(this->input_buffer()), this->input_buffer_size());

        auto p = this->buffer_.find('\n');
        if (std::string::npos != p) {
          auto result = this->buffer_.substr(0, p);
          this->buffer_.erase(0, p + 1);

          if (this->done_callback_) {
            std::function<void(const vds::service_provider & sp, const std::string &)> f;
            this->done_callback_.swap(f);
            f(sp, result);
          }
        }
      }

      return this->input_buffer_size();
    }
        
  private:
    std::string buffer_;
    std::function<void(const vds::service_provider & sp, const std::string &)> done_callback_;

    template <typename done_method>
    void send_terminator(done_method & done, handler & owner)
    {
      owner.next(done, std::string());
    }
  };
private:
  std::function<void(const vds::service_provider & sp, const std::string &)> done_callback_;
};

TEST(network_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::file_logger file_logger(vds::ll_trace);

  registrator.add(file_logger);
  registrator.add(mt_service);
  registrator.add(network_service);

  vds::barrier done;

  auto sp = registrator.build("network_tests::test_server");
  registrator.start(sp);
  vds::imt_service::enable_async(sp);

  std::shared_ptr<std::exception> error;
  vds::barrier b;
  vds::tcp_socket_server server;
  server.start(
    sp,
    "127.0.0.1",
    8000,
    [&error](const vds::service_provider & sp, const vds::tcp_network_socket & s) {
    vds::cancellation_token_source cancellation;
    vds::dataflow(
      vds::read_tcp_network_socket(s, cancellation.token()),
      vds::write_tcp_network_socket(s, cancellation.token())
    )(
      [](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Server closed");
    },
      [&error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      error = ex;
    },
      sp);
  }).wait(
    [&b](const vds::service_provider & sp) {
    sp.get<vds::logger>()->debug(sp, "Server has been started");
    b.set();
  },
    [&b, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
    error = ex;
    b.set();
  },
    sp);

  b.wait();
  b.reset();

  std::string answer;

  vds::tcp_network_socket::connect(
    sp,
    (const char *)"127.0.0.1",
    8000)
    .then(
      [&b, &answer](
        const std::function<void(const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        vds::tcp_network_socket && s) {

    sp.get<vds::logger>()->debug(sp, "Connected");
    vds::cancellation_token_source cancellation;
    random_buffer data;

    vds::async_series(
      vds::create_async_task(
        [s, &data, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {


      vds::dataflow(
        random_reader<uint8_t>(data.data(), data.size()),
        vds::write_tcp_network_socket(s, cancellation.token())
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
        [s, &data, &answer, cancellation](const std::function<void(const vds::service_provider & sp)> & done, const vds::error_handler & on_error, const vds::service_provider & sp) {
      vds::dataflow(
        vds::read_tcp_network_socket(s, cancellation.token()),
        compare_data<uint8_t>(data.data(), data.size())
      )(
        [done](const vds::service_provider & sp) {
        done(sp);
      },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
        sp.get<vds::logger>()->debug(sp, "Client reader closed");
        on_error(sp, ex);
      },
        sp.create_scope("Client read dataflow"));

    })
      ).wait(
        [done](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Client reader closed");
      done(sp);
    },
        [on_error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      sp.get<vds::logger>()->debug(sp, "Client reader error");
      on_error(sp, ex);
    },
      sp.create_scope("Client reader"));
  }).wait(
    [&b](const vds::service_provider & sp) {
      sp.get<vds::logger>()->debug(sp, "Request sent");
      b.set();
    },
    [&b, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      error = ex;
      sp.get<vds::logger>()->debug(sp, "Request error");
      b.set();
    },
    sp.create_scope("Client"));

  b.wait();
  //Wait
  registrator.shutdown(sp);

  if (error) {
    GTEST_FAIL() << error->what();
  }
}
