// test_vds_network.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

class echo_server
{
public:
   
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
      const echo_server & owner)
    {
    }
    
    void operator()(const vds::network_socket & s) const {
      std::cout << "new connection\n";
    
      //echo
      vds::pipeline(
        vds::input_network_stream(s),//input
        vds::output_network_stream(s)//output
        )
      (
       []() {
       },
       [](std::exception * ex) {
         FAIL() << ex->what();
         delete ex;
       }
      );
    }
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
    : write_task_(done, on_error, owner.s_),
      data_(owner.data_)
    {
    }
    
    void operator()() {
      this->write_task_.set_data(
        this->data_.c_str(),
        this->data_.length());
      this->write_task_.schedule();
    }
  private:
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
        auto result = this->buffer_.substr(0, p - 1);
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
        
        vds::pipeline(
          vds::socket_server(sp, "127.0.0.1", 8000),
          echo_server()
        )(
          []() {
          },
          [](std::exception * ex){
            FAIL() << ex->what();
            delete ex;
          }
        );
        
        vds::sequence(
          vds::socket_connect()
        )
        ([&done](const vds::network_socket & s){
          
          vds::sequence(
            send_test(s, "test\n")
          )(
            []() {},
            [](std::exception * ex) {
              FAIL() << ex->what();
              delete ex;
          });

          vds::pipeline(
            vds::input_network_stream(s),
            read_for_newline(),
            check_result("test")
          )
          ([&done]() {
            done.set();
           },
           [&done](std::exception * ex) {
            FAIL() << ex->what();
            delete ex;
            done.set();
          });
        },
        [&done](std::exception * ex) {
          FAIL() << ex->what();
          delete ex;
          done.set();
        },
        "127.0.0.1",
        8000);
    }

    done.wait();

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


