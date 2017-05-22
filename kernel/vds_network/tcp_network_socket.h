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
    
    static async_task<tcp_network_socket> connect(
      const service_provider & sp,
      const std::string & server,
      const uint16_t port);
    
    _tcp_network_socket * operator -> () const { return this->impl_.get(); }
    
  private:
    friend class _tcp_network_socket;
    tcp_network_socket(const std::shared_ptr<_tcp_network_socket> & impl);
    std::shared_ptr<_tcp_network_socket> impl_;
  };
  
  class read_tcp_network_socket
  {
  public:
    read_tcp_network_socket(
      const tcp_network_socket & s)
    : s_(s)
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
        s_(args.s_),
        reader_(args.create_reader(
          [this](const vds::service_provider & sp, size_t readed){
            this->processed(sp, readed);
          },
          [this](const vds::service_provider & sp, std::exception_ptr ex){
            this->error(sp, ex);
          }))
      {
      }
      
      void async_get_data(const vds::service_provider & sp)
      {
        read_async(sp, this->reader_, this->output_buffer_, this->output_buffer_size_);
      }
      
    private:
      tcp_network_socket s_;
      _read_socket_task * const reader_;      
    };
  private:
    tcp_network_socket s_;
    
    _read_socket_task * create_reader(
      const std::function<void(const service_provider & sp, size_t readed)> & readed_method,
      const error_handler & error_method) const;
      
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
      const tcp_network_socket & s)
    : s_(s)
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
        s_(args.s_),
        writer_(args.create_writer(
          [this](const vds::service_provider & sp, size_t written){
            this->processed(sp, written);
          },
          [this](const vds::service_provider & sp, std::exception_ptr ex){
            this->error(sp, ex);
          }))
      {
      }
      
      void async_push_data(const vds::service_provider & sp)
      {
        write_async(sp, this->writer_, this->input_buffer_, this->input_buffer_size_);
      }
      
    private:
      tcp_network_socket s_;
      _write_socket_task * const writer_;
    };
  private:
    tcp_network_socket s_;
    
    _write_socket_task * create_writer(
      const std::function<void(const service_provider & sp, size_t written)> & write_method,
      const error_handler & error_method) const;
      
    static void write_async(
      const service_provider & sp,
      _write_socket_task * writer,
      const void * buffer,
      size_t buffer_size);
      
    static void destroy_writer(_write_socket_task * writer);
    
  };
}

#endif//__VDS_NETWORK_NETWORK_SOCKET_H_