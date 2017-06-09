#ifndef __VDS_NETWORK_NETWORK_SOCKET_H_
#define __VDS_NETWORK_NETWORK_SOCKET_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <memory>
#include "dataflow.h"
#include "async_task.h"

namespace vds {
  class _tcp_network_socket;
  class _read_socket_task;
  class _write_socket_task;
  
  class tcp_network_socket
  {
  public:
    tcp_network_socket();
    ~tcp_network_socket();
    
    static async_task<tcp_network_socket &&> connect(
      const service_provider & sp,
      const std::string & server,
      const uint16_t port);
    
    _tcp_network_socket * operator -> () const { return this->impl_.get(); }

    void close();
    
  private:
    friend class _tcp_network_socket;
    tcp_network_socket(const std::shared_ptr<_tcp_network_socket> & impl);
    std::shared_ptr<_tcp_network_socket> impl_;
  };
  
  class read_tcp_network_socket
  {
  public:
    read_tcp_network_socket(
      const tcp_network_socket & s,
      const cancellation_token & cancel_token)
    : s_(s), cancel_token_(cancel_token)
    {
    }
    
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;
    
    template <typename context_type>
    class handler : public vds::async_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = vds::async_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const read_tcp_network_socket & args)
        : base_class(context),
        s_(args.s_), cancel_token_(args.cancel_token_)
      {
        this->reader_ = args.create_reader(
          [this](const vds::service_provider & sp, size_t readed) {
            if (this->processed(sp, readed)) {
              read_async(sp, this->reader_, this->output_buffer(), this->output_buffer_size());
            }
          },
          [this](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          this->error(sp, ex);
          },
          this->cancel_token_);
      }

      ~handler()
      {
        destroy_reader(this->reader_);
      }
      
      void async_get_data(const vds::service_provider & sp)
      {
        read_async(sp, this->reader_, this->output_buffer(), this->output_buffer_size());
      }
      
    private:
      tcp_network_socket s_;
      cancellation_token cancel_token_;
      _read_socket_task * reader_;
    };
  private:
    tcp_network_socket s_;
    cancellation_token cancel_token_;
    
    _read_socket_task * create_reader(
      const std::function<void(const service_provider & sp, size_t readed)> & readed_method,
      const error_handler & error_method,
      const cancellation_token & cancel_token) const;
      
    static void read_async(
      const service_provider & sp,
      _read_socket_task * reader,
      void * buffer,
      size_t buffer_size);
    
    static void destroy_reader(_read_socket_task * reader);
    
  };
  class write_tcp_network_socket
  {
  public:
    write_tcp_network_socket(
      const tcp_network_socket & s,
      const cancellation_token & cancel_token)
    : s_(s), cancel_token_(cancel_token)
    {
    }
    
    using incoming_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;
    
    template <typename context_type>
    class handler : public vds::async_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = vds::async_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const write_tcp_network_socket & args)
        : base_class(context),
        s_(args.s_), cancel_token_(args.cancel_token_)
      {
        this->writer_ = args.create_writer(
          [this](const vds::service_provider & sp, size_t written) {
          if (this->processed(sp, written)) {
            write_async(sp, this->writer_, this->input_buffer(), this->input_buffer_size());
          }
        },
          [this](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          this->error(sp, ex);
        },
          this->cancel_token_);
      }

      ~handler()
      {
        destroy_writer(this->writer_);
      }
      
      void async_push_data(const vds::service_provider & sp)
      {
        write_async(sp, this->writer_, this->input_buffer(), this->input_buffer_size());
      }
      
    private:
      tcp_network_socket s_;
      cancellation_token cancel_token_;
      _write_socket_task * writer_;
    };
  private:
    tcp_network_socket s_;
    cancellation_token cancel_token_;
    
    _write_socket_task * create_writer(
      const std::function<void(const service_provider & sp, size_t written)> & write_method,
      const error_handler & error_method,
      const cancellation_token & cancel_token) const;
      
    static void write_async(
      const service_provider & sp,
      _write_socket_task * writer,
      const void * buffer,
      size_t buffer_size);
      
    static void destroy_writer(_write_socket_task * writer);
    
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_