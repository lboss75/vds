// test_vds_network.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

class echo_server
{
public:
   
  template<
    typename done_method_type,
    typename error_method_type
  >
  class handler
  {
  public:
    handler(
      const done_method_type & done,
      const error_method_type & on_error,
      const echo_server & owner)
    {
    }
    
    void operator()(const network_socket & s) const {
      std::cout << "new connection\n";
    
      //echo
      auto p = vds::pipeline(
        vds::network_stream(nm, s),//input
        network_stream(nm, s)//output
        );
    }
  };
};

class send_test
{
public:
  send_test(const std::string & testdata)
  {
  }

  template<
    typename done_method_type,
    typename error_method_type,
    typename next_method_type
  >
  class handler
  {
  public:
    handler(
      const done_method_type & done,
      const error_method_type & on_error,
      const next_method_type & next,
      const send_test & owner)
    {
    }
    
    void operator()(
      const vds::network_socket & socket
    ) {
      socket.write_async(
        this->done_,
        this->on_error_,
        sp,
        this->data_->c_str(),
        this->data_->length());
      
      this->next_();
    }
  private:
    const done_method_type & done_;
    const error_method_type & on_error_;
    const next_method_type & next_;
    std::string data_;
  };
};

class read_for_newline
{
public:
  read_for_newline(
    const vds::service_provider & sp,
    const char * input_buffer
  ) : sp_(sp), input_buffer_(input_buffer)
  {
  }
  template<
    typename done_method_type,
    typename error_method_type,
    typename next_method_type
  >
  class handler
  {
  public:
    handler(
      const done_method_type & done,
      const error_method_type & on_error,
      const next_method_type & next,
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
        this->buffer_.remove(0, p + 1);
        this->next_(result);
      }
    }
    
  private:
    const done_method_type & done_;
    const error_method_type & on_error_;
    const next_method_type & next_;
    std::string buffer_;
      
    void process(
      const char * data,
      size_t len
    ) {
      if (len > 0) {
          size_t start = 0;
          while (start < len && data[start] != '\n') {
              ++start;
          }

          this->buffer_ += std::string(data, start);

          if (start < len) {
              ++start;//skip \n
              len -= start;
              data += start;
              
              auto result = this->buffer_;
              this->buffer_.clear();
              
              this->next_(result);
          }
          else {
              done();
          }
      }
      else {
          done();
      }
    }
    
  };
  
private:  
  vds::service_provider sp_;
  const char * input_buffer_;
};

std::function<void(const std::function<void(void)> &, const vds::error_handler_t & , const std::string &)>
check_string(const std::string & value, const std::function<void(void)> & done_check) {
  return [value, done_check](const std::function<void(void)> & done, const vds::error_handler_t & on_error, const std::string & data){
    if (value == data) {
      done_check();
    }
    else {
      throw new std::runtime_error("invalid data: exprected '" + data + "', getted '" + value + "'");
    }
  };
}

std::function<void(const std::function<void(void)> &, const vds::error_handler_t &)>
check_result(
  const vds::service_provider & sp,
  const char * data,
  const vds::network_socket & socket) {
  return [sp, data, socket](const std::function<void(void)> & done, const vds::error_handler_t & on_error) {
    std::shared_ptr<std::vector<uint8_t>> buffer(new std::vector<uint8_t>(1024));
    vds::barrier done_event;
    
    vds::pipeline(
      [sp, socket, buffer](const std::function<void(size_t)> & done, const vds::error_handler_t & on_error) {
          socket.read_async(done, on_error, sp, buffer->data(), buffer->size());
      },
      read_for_newline(sp, (const char *)buffer->data()),
      check_string(data, [&done_event] {
        done_event.set();
      })
        )([]() {}, on_error);

    done_event.wait();
    done();
  };
}

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
          socket_server("127.0.0.1", 8000),
          echo_server()
          [](
            const std::function<void(void)> & done,
            const std::function<void(std::exception *)> & on_error,
            vds::network_socket & s) {
            std::cout << "new connection\n";
          
            //echo
            auto p = vds::pipeline(
              network_stream(nm, s),//input
              network_stream(nm, s)//output
              );
            
            p([ns = std::move(s)]{
            },
            on_error);
            
            done();
         )(
           
        );

        
        sp.get<vds::inetwork_manager>()
          .start_server(sp, "127.0.0.1", 8000, 
          [sp, &done, &error, nm](
            const vds::network_socket & s) {
            std::cout << "new connection\n";

            std::shared_ptr<std::vector<uint8_t>> buffer(new std::vector<uint8_t>(1024));
            vds::pipeline(
              std::function<void(const std::function<void(size_t)> &, const vds::error_handler_t &)>(
              [sp, s, buffer](const std::function<void(size_t)> & done, const vds::error_handler_t & on_error){
                s.read_async(done, on_error, sp, buffer->data(), buffer->size());
              }),
              [sp, buffer, s](const std::function<void(void)> & done, const vds::error_handler_t & on_error, size_t readed) {
                if (0 == readed) {//closed connection
                }
                else {
                  std::cout << "readed " << readed << " bytes (" 
                  << std::string((const char *)buffer->data(), readed) << ")\n";
                    s.write_async(done, on_error, sp, buffer->data(), readed);
                }
            });
        },
          [](std::exception * ex) {
        });

        auto s = sp.get<vds::inetwork_manager>().connect("127.0.0.1", 8000);
        vds::sequence(
          send_test(sp, "test\n", s),
          check_result(sp, "test", s)
        )([&done]{
          done.set();
        }, [](std::exception * ex) {

        });
    }

    done.wait();

    registrator.shutdown();
}

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

int main(int argc, char **argv) {
    setlocale(LC_ALL, "Russian");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


