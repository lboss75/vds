/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "network_socket.h"
class echo_server
{
public:
   
  template<typename context_type>
  class handler : public vds::sequence_step<context_type, void (void)>
  {
    using base_class = vds::sequence_step<context_type, void (void)>;
  public:
    handler(
      const context_type & context,
      const echo_server & owner)
      : base_class(context)
    {
    }
    
    void operator()(vds::network_socket & s) {
      (new connection_handler<
        typename base_class::next_step_t,
        typename base_class::error_method_t>(
        this->next, this->error, s))->start();
    }
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
      vds::network_socket & s
    ) : 
      next_(this, next),
      error_(this, error),
      s_(std::move(s))
    {
    }

    void start() {
      vds::sequence(
        vds::input_network_stream(this->s_),//input
        vds::output_network_stream(this->s_)//output
      )
      (
        this->next_,
        this->error_
        );
    }
  private:
    vds::auto_cleaner<connection_handler, next_method_type> next_;
    vds::auto_cleaner<connection_handler, error_method_type> error_;

    vds::network_socket s_;
  };

};

class send_test
{
public:
  send_test(const vds::network_socket & s, const std::string & testdata)
  : s_(s), data_(testdata)
  {
  }

  template<typename context_type>
  class handler : public vds::sequence_step<context_type, void(void)>
  {
    using base_class = vds::sequence_step<context_type, void(void)>;
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
    
    void operator()() {
      this->write_task_.schedule(this->s_);
    }

    void processed()
    {
      this->next();
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
  class handler : public vds::sequence_step<context_type, void(const std::string &)>
  {
  public:
    handler(
      const context_type & context,
      const read_for_newline & owner)
    : vds::sequence_step<context_type, void(const std::string &)>(context)
    {
    }
  
    void operator()(
      const void * data,
      size_t len
    )
    {
      if(0 == len) {
        this->next(this->buffer_);
      }
      else {
        this->buffer_.append(
          reinterpret_cast<const char *>(data),
          len);

        auto p = this->buffer_.find('\n');
        if (std::string::npos == p) {
          this->prev();
          return;
        }
        else {
          auto result = this->buffer_.substr(0, p);
          this->buffer_.erase(0, p + 1);
          this->next(result);
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
  class handler : public vds::sequence_step<context_type, void(void)>
  {
  public:
    handler(
      const context_type & context,
      const check_result & owner)
    : vds::sequence_step<context_type, void(void)>(context),
      data_(owner.data_)
    {
    }
  
    void operator()(
      const std::string & data
    )
    {
      if(this->data_ == data){
        this->next();
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
  class handler : public vds::sequence_step<context_type, void(void)>
  {
  public:
    handler(
      const context_type & context,
      const socket_client & args
    )
      : vds::sequence_step<context_type, void(void)>(context)
    {
    }

    void operator()(vds::network_socket & s)
    {
      this->s_ = std::move(s);

      vds::barrier b;
      auto done_handler = vds::lambda_handler(
       [&b]() {
        b.set();
        }
      );
      auto error_handler = vds::lambda_handler(
        [&b](std::exception * ex) {
          FAIL() << ex->what();
          delete ex;
          b.set();
      }
      );
      
      vds::write_socket_task<
        decltype(done_handler),
        decltype(error_handler)>
        write_task(done_handler, error_handler);
      const char data[] = "test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_\n";
      write_task.set_data(data, sizeof(data) - 1);
      write_task.schedule(this->s_.handle());

      vds::sequence(
        vds::input_network_stream(this->s_),
        read_for_newline(),
        check_result("test_test_test_test_test_test_test_test_test_test_test_test_test_test_test_")
      )
      (
        this->next,
        this->error
      );

      b.wait();
    }
    
    void processed()
    {
    }
    
  private:
    vds::network_socket s_;
  };
};

TEST(network_tests, test_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::console_logger console_logger;

    registrator.add(console_logger);
    registrator.add(network_service);

    vds::barrier done;

    {
        auto sp = registrator.build();
        
        auto done_server = vds::lambda_handler(
          []() {
          }
        );
        auto error_server = vds::lambda_handler(
          [](std::exception * ex){
            FAIL() << ex->what();
            delete ex;
          }
        );
        
        vds::sequence(
          vds::socket_server(sp, "127.0.0.1", 8000),
          echo_server()
        )(
            done_server,
            error_server
        );

        auto done_client = vds::lambda_handler(
          [&done]() {
            done.set();
          }
        );
        auto error_client = vds::lambda_handler(
          [&done](std::exception * ex) {
            FAIL() << ex->what();
            delete ex;
            done.set();
          }
        );

        
        vds::sequence(
          vds::socket_connect(sp),
          socket_client()
        )
        (
          done_client,
          error_client,
          "127.0.0.1",
          8000);

        done.wait();
    }


    registrator.shutdown();
}

TEST(network_tests, test_udp_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::console_logger console_logger;

    registrator.add(console_logger);
    registrator.add(network_service);

    vds::barrier done;

    {
      auto sp = registrator.build();

      auto done_server = vds::lambda_handler([](size_t size) {
      });

      auto error_server = vds::lambda_handler([](std::exception * ex) {
        FAIL() << "Client error " << ex->what();
      });

      vds::udp_socket server_socket(sp);
      vds::sequence(
        vds::udp_server(sp, server_socket, "127.0.0.1", 8001),
        vds::udp_receive(sp, server_socket),
        vds::udp_send(server_socket)
      )
      (
        done_server,
        error_server
        );


      const char testdata[] = "testdata";
      auto done_client = vds::lambda_handler([&done, &testdata](const sockaddr_in & addr, const void * data, size_t size) {
        if (std::string((const char *)data, size) == testdata) {
          done.set();
        }
        else {
          FAIL() << "Invalid data readed " << std::string((const char *)data, size);
        }
      });

      auto error_client = vds::lambda_handler([](std::exception * ex) {
        FAIL() << "Client error " << ex->what();
      });

      Sleep(5000);//Waiting start UDP server

      vds::udp_socket client_socket(sp);
      vds::sequence(
        vds::udp_client(sp, client_socket, "127.0.0.1", 8001, testdata, sizeof(testdata) - 1),
        vds::udp_receive(sp, client_socket)
      )
      (
        done_client,
        error_client
        );

      done.wait();
    }


    registrator.shutdown();
}

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


