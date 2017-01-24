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
  public:
    handler(
      context_type & conext,
      const echo_server & owner)
      : base(context)
    {
    }
    
    void operator()(vds::network_socket && s) {
      std::cout << "new connection\n";

      (new connection_handler<
        next_step_t,
        error_method_t>(
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
    ) : s_(std::move(s)),
      next_(this, next),
      error_(this, error)
    {
    }

    void start() {
      vds::pipeline(
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
  : data_(testdata), s_(s)
  {
  }

  template<
    typename done_method_type,
    typename error_method_type
  >
  class handler
  {
  public:
    handler(
      done_method_type & done,
      error_method_type & on_error,
      const send_test & owner)
    : write_task_(done, on_error),
      s_(owner.s_.handle()),
      data_(owner.data_)
    {
    }
    
    void operator()() {
      this->write_task_.set_data(
        this->data_.c_str(),
        this->data_.length());
      this->write_task_.schedule(this->s_);
    }
  private:
    vds::network_socket::SOCKET_HANDLE s_;
    vds::write_socket_task<done_method_type, error_method_type> write_task_;
    std::string data_;
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
  template<
    typename done_method_type,
    typename next_method_type,
    typename error_method_type
  >
  class handler
  {
  public:
    handler(
      done_method_type & done,
      next_method_type & next,
      error_method_type & on_error,
      const read_for_newline & owner)
    : done_(done), on_error_(on_error),
    next_(next)
    {
    }
  
    void operator()(
      const void * data,
      size_t len
    )
    {
      this->buffer_.append(
        reinterpret_cast<const char *>(data),
        len);
      this->processed();
    }
    
    void processed()
    {
      auto p = this->buffer_.find('\n');
      if(std::string::npos == p) {
        this->done_();
      }
      else {
        auto result = this->buffer_.substr(0, p);
        this->buffer_.erase(0, p + 1);
        this->next_(result);
      }
    }
    
  private:
    done_method_type & done_;
    error_method_type & on_error_;
    next_method_type & next_;
    std::string buffer_;
  };
};

class check_result
{
public:
  check_result(const std::string & data)
  : data_(data)
  {
  }
  
  template<
    typename done_method_type,
    typename next_method_type,
    typename error_method_type
  >
  class handler
  {
  public:
    handler(
      done_method_type & done,
      next_method_type & next,
      error_method_type & on_error,
      const check_result & owner)
    : done_(done), on_error_(on_error),
    next_(next), data_(owner.data_)
    {
    }
  
    void operator()(
      const std::string & data
    )
    {
      if(this->data_ == data){
        this->next_();
      }
      else {
        ASSERT_EQ(this->data_, data);
      }
    }
    
    void processed()
    {
      this->done_();
    }
    
  private:
    done_method_type & done_;
    error_method_type & on_error_;
    next_method_type & next_;
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

  template <
    typename context_type
  >
  class handler
  {
  public:
    handler(
      done_method_type & done_method,
      error_method_type & error_method,
      const socket_client & args
    ): done_method_(done_method), error_method_(error_method)
    {
    }

    void operator()(vds::network_socket & s)
    {
      std::cout << "server connected\n";
      this->s_ = std::move(s);

      vds::sequence(
        send_test(this->s_, "test\n")
      )(
        []() {
        std::cout << "test sent\n";
      },
        [](std::exception * ex) {
        FAIL() << ex->what();
        delete ex;
      });

      vds::pipeline(
        vds::input_network_stream(this->s_),
        read_for_newline(),
        check_result("test")
      )
      (
        this->done_method_,
        this->error_method_
      );
    }
  private:
    vds::network_socket s_;
    done_method_type & done_method_;
    error_method_type & error_method_;
  };
};

TEST(network_tests, test_server)
{
    vds::service_registrator registrator;

    vds::network_service network_service;
    vds::mt_service mt_service;
    vds::console_logger console_logger;

    registrator.add(console_logger);
    registrator.add(mt_service);
    registrator.add(network_service);

    vds::barrier done;
    std::exception * error = nullptr;

    {
        auto sp = registrator.build();
        auto nm = sp.get<vds::inetwork_manager>();
        
        vds::sequence(
          vds::socket_server(sp, "127.0.0.1", 8000),
          echo_server()
        )(
          []() {
            std::cout << "server closed\n";
          },
          [](std::exception * ex){
            FAIL() << ex->what();
            delete ex;
          }
        );
        
        vds::sequence(
          vds::socket_connect(sp),
          socket_client()
        )
        ([&done]() {
          std::cout << "check done\n";
          done.set();
        },
        [&done](std::exception * ex) {
          FAIL() << ex->what();
          delete ex;
          done.set();
        },
        "127.0.0.1",
        8000);

        done.wait();
    }


    registrator.shutdown();
}
/*
TEST(network_tests, test_udp_server)
{
    vds::service_registrator registrator([](std::exception * ex) {
        FAIL() << ex->what();
        delete ex;
    });

    vds::network_service network_service;
    vds::mt_service mt_service;
    vds::console_logger console_logger;

    registrator.add(console_logger);
    registrator.add(mt_service);
    registrator.add(network_service);

    vds::barrier done;

    {
        auto sp = registrator.build();
        sp.get<vds::inetwork_manager>().bind(sp, "127.0.0.1", 8000,
            [sp, &done](const vds::udp_socket & s, const sockaddr_in & address, const void * buffer, size_t readed) {
            std::cout << "new connection\n";

            std::cout << "readed " << readed << " bytes ("
                << std::string((const char *)buffer, readed) << ")\n";

            s.write_async([](size_t) {}, [](std::exception *) {}, sp, address, buffer, readed);
        }, [](std::exception *) {});

        auto s = sp.get<vds::inetwork_manager>().bind(sp, "127.0.0.1", 8001,
            [&done](const vds::udp_socket &s, const sockaddr_in & addr, const void * data, size_t size)
            {
                if (std::string((const char *)data, size) == "test") {
                    done.set();
                }
                else {
                    FAIL() << "Invalid data readed " << std::string((const char *)data, size);
                }
            },
          [](std::exception *) {}
        );

        struct sockaddr_in serveraddr;
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
        serveraddr.sin_port = htons(8000);

        s.write_async(
          [](size_t len) {
            std::cout << "sent " << len << " bytes\n";
          },
          [](std::exception *) {},
          sp,
          serveraddr,
          "test",
          4);
        
      done.wait();
    }


    registrator.shutdown();
}
*/
int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


