/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "network_socket.h"
class echo_server
{
public:
  echo_server(const vds::service_provider & sp)
  : sp_(sp)
  {
  }
   
  template<typename context_type>
  class handler : public vds::dataflow_step<context_type, void (void)>
  {
    using base_class = vds::dataflow_step<context_type, void (void)>;
  public:
    handler(
      const context_type & context,
      const echo_server & owner)
      : base_class(context),
      sp_(owner.sp_)
    {
    }
    
    void operator()(const vds::service_provider & sp, vds::network_socket * s) {
      (new connection_handler<
        typename base_class::next_step_t,
        typename base_class::error_method_t>(
          this->next, this->error, s))->start(sp);
    }
  private:
    vds::service_provider sp_;
  };

  template<
    typename next_method_type,
    typename error_method_type
  >
  class connection_handler
  {
  public:
    connection_handler(
      next_method_type & next, 
      error_method_type & error,
      vds::network_socket * s
    ) : 
      next_(this, next),
      error_(this, error),
      s_(std::move(*s))
    {
    }

    void start(const vds::service_provider & sp) {
      vds::dataflow(
        vds::input_network_stream(this->s_),//input
        vds::output_network_stream(this->s_)//output
      )
      (
        this->next_,
        this->error_,
        sp);
    }
  private:
    vds::auto_cleaner<connection_handler, next_method_type> next_;
    vds::auto_cleaner<connection_handler, error_method_type> error_;

    vds::network_socket s_;
  };
  
private:
  vds::service_provider sp_;
};

class send_test
{
public:
  send_test(const vds::network_socket & s, const std::string & testdata)
  : s_(s), data_(testdata)
  {
  }

  template<typename context_type>
  class handler : public vds::dataflow_step<context_type, void(void)>
  {
    using base_class = vds::dataflow_step<context_type, void(void)>;
  public:
    handler(
      context_type & context,
      const send_test & owner)
    : base_class(context),
      s_(owner.s_.handle()),
      data_(owner.data_),
      write_task_(*this, this->error)
    {
      this->write_task_.set_data(
        this->data_.c_str(),
        this->data_.length());
    }
    
    void operator()(const vds::service_provider & sp) {
      this->write_task_.schedule(this->s_);
    }

    void processed(const vds::service_provider & sp)
    {
      this->next(sp);
    }
  private:
    vds::network_socket::SOCKET_HANDLE s_;
    std::string data_;
    vds::write_socket_task<
      handler,
      typename base_class::error_method_t> write_task_;
  };
  
private:
  const vds::network_socket & s_;
  std::string data_;
};

class read_for_newline
{
public:
  read_for_newline()
  {
  }

  template<typename context_type>
  class handler : public vds::dataflow_step<context_type, void(const std::string &)>
  {
  public:
    handler(
      const context_type & context,
      const read_for_newline & owner)
    : vds::dataflow_step<context_type, void(const std::string &)>(context)
    {
    }
  
    void operator()(
      const vds::service_provider & sp,
      const void * data,
      size_t len
    )
    {
      if(0 == len) {
        this->next(sp, this->buffer_);
      }
      else {
        this->buffer_.append(
          reinterpret_cast<const char *>(data),
          len);

        auto p = this->buffer_.find('\n');
        if (std::string::npos == p) {
          this->prev(sp);
          return;
        }
        else {
          auto result = this->buffer_.substr(0, p);
          this->buffer_.erase(0, p + 1);
          this->next(sp, result);
        }
      }
    }

    
  private:
    std::string buffer_;

    template <typename done_method>
    void send_terminator(done_method & done, handler & owner)
    {
      owner.next(done, std::string());
    }
  };
};

class check_result
{
public:
  check_result(const std::string & data)
  : data_(data)
  {
  }
  
  template<typename context_type>
  class handler : public vds::dataflow_step<context_type, void(void)>
  {
  public:
    handler(
      const context_type & context,
      const check_result & owner)
    : vds::dataflow_step<context_type, void(void)>(context),
      data_(owner.data_)
    {
    }
  
    void operator()(
      const vds::service_provider & sp,
      const std::string & data
    )
    {
      if(this->data_ == data){
        this->next(sp);
      }
      else {
        ASSERT_EQ(this->data_, data);
      }
    }
    
  private:
    std::string data_;
  };
  
private:
  std::string data_;
};

class socket_client
{
public:
  socket_client()
  {
  }

  template <typename context_type>
  class handler : public vds::dataflow_step<context_type, void(void)>
  {
  public:
    handler(
      const context_type & context,
      const socket_client & args
    )
      : vds::dataflow_step<context_type, void(void)>(context)
    {
    }

    void operator()(const vds::service_provider & sp, vds::network_socket & s)
    {
      this->s_ = std::move(s);

      vds::barrier b;
      auto done_handler = vds::lambda_handler(
       [&b](const vds::service_provider & sp) {
        b.set();
        }
      );
      auto error_handler = vds::lambda_handler(
        [&b](const vds::service_provider & sp, std::exception_ptr ex) {
          FAIL() << vds::exception_what(ex);
          b.set();
      }
      );
      
      vds::write_socket_task<
        decltype(done_handler),
        decltype(error_handler)>
        write_task(done_handler, error_handler);
      const char data[] = "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_\n";
      write_task.set_data(data, sizeof(data) - 1);
      write_task.schedule(sp, this->s_.handle());

      vds::dataflow(
        vds::input_network_stream(this->s_),
        read_for_newline(),
        check_result("test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_")
      )
      (
        this->next,
        this->error,
        sp
      );

      b.wait();
    }
    
    void processed(const vds::service_provider & sp)
    {
    }
    
  private:
    vds::network_socket s_;
  };
};

TEST(network_tests, test_server)
{
  vds::service_registrator registrator;

  vds::mt_service mt_service;
  vds::network_service network_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(mt_service);
  registrator.add(network_service);

  vds::barrier done;

  {
    auto sp = registrator.build("network_tests::test_server");

    auto done_server = vds::lambda_handler(
      [](const vds::service_provider & sp) {
    }
    );
    auto error_server = vds::lambda_handler(
      [](const vds::service_provider & sp, std::exception_ptr ex) {
      FAIL() << vds::exception_what(ex);
    }
    );

    vds::dataflow(
      vds::socket_server(sp, "127.0.0.1", 8000),
      echo_server(sp)
    )(
      done_server,
      error_server,
      sp
      );

    auto done_client = vds::lambda_handler(
      [&done](const vds::service_provider & sp) {
      done.set();
    }
    );
    auto error_client = vds::lambda_handler(
      [&done](const vds::service_provider & sp, std::exception_ptr ex) {
      done.set();
      FAIL() << vds::exception_what(ex);
    }
    );


    vds::dataflow(
      vds::socket_connect(),
      socket_client()
    )
    (
      done_client,
      error_client,
      sp,
      "127.0.0.1",
      8000);

    done.wait();

    registrator.shutdown(sp);
  }
}

TEST(network_tests, test_udp_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::mt_service mt_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(mt_service);
    registrator.add(network_service);

    vds::barrier done;

    {
      auto sp = registrator.build("network_tests.test_udp_server");

      auto done_server = vds::lambda_handler([](const vds::service_provider & sp) {
      });

      auto error_server = vds::lambda_handler([](const vds::service_provider & sp, std::exception_ptr ex) {
        FAIL() << "Client error " << vds::exception_what(ex);
      });

      vds::udp_socket server_socket;
      vds::dataflow(
        vds::udp_server(sp, server_socket, "127.0.0.1", 8001),
        vds::udp_receive(sp, server_socket),
        vds::udp_send(sp, server_socket)
      )
      (
        done_server,
        error_server,
        sp);


      const char testdata[] = "testdata";
      auto done_client = vds::lambda_handler([&done, &testdata](const vds::service_provider & sp, const sockaddr_in * addr, const void * data, size_t size) {
        if (std::string((const char *)data, size) == testdata) {
          done.set();
        }
        else {
          FAIL() << "Invalid data readed " << std::string((const char *)data, size);
        }
      });

      auto error_client = vds::lambda_handler([](const vds::service_provider & sp, std::exception_ptr ex) {
        FAIL() << "Client error " << vds::exception_what(ex);
      });
      std::this_thread::sleep_for(std::chrono::seconds(5));//Waiting start UDP server

      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = inet_addr("127.0.0.1");
      addr.sin_port = htons(8001);

      vds::udp_socket client_socket;
      vds::dataflow(
        vds::udp_send(sp, client_socket),
        vds::udp_receive(sp, client_socket)
      )
      (
        done_client,
        error_client,
        sp,
        &addr,
        (const void *)testdata,
        sizeof(testdata) - 1
        );

      done.wait();

      registrator.shutdown(sp);
    }
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


