#ifndef __VDS_CORE_ASYNC_STREAM_H_
#define __VDS_CORE_ASYNC_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <mutex>

#include "dataflow.h"
#include "async_task.h"
#include "mt_service.h"

namespace vds {

  template <typename item_type>
  class async_stream
  {
  public:
    async_stream()
      : second_(0), front_(0), back_(0), eof_(false)
    {
    }

    async_task<> write_all_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      return create_async_task(
        [this, data, data_size](
          const std::function<void(const service_provider & sp)> & done,
          const error_handler & on_error,
          const service_provider & sp) {
          
          this->write_all(sp, done, data, data_size);
        });
    }
    
    async_task<size_t> write_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      return create_async_task(
        [this, data, data_size](
          const std::function<void(const service_provider & sp, size_t written)> & done,
          const error_handler & on_error,
          const service_provider & sp) {

          if(0 == data_size) {
            this->eof_ = true;
            if(this->continue_read_){
              std::function<void(void)> f;
              this->continue_read_.swap(f);
              mt_service::async(sp, f);
            }
          } else {
            this->continue_write(sp, done, data, data_size);
          }
      });
    }

    async_task<size_t /*readed*/> read_async(const service_provider & sp, item_type * buffer, size_t buffer_size)
    {
      return create_async_task(
        [this, buffer, buffer_size](
          const std::function<void(const service_provider & sp, size_t readed)> & done,
          const error_handler & on_error,
          const service_provider & sp) {

        this->continue_read(sp, done, buffer, buffer_size);
      });
    }

  private:
    std::mutex buffer_mutex_;
    item_type buffer_[4096];
    uint32_t second_;
    uint32_t front_;
    uint32_t back_;
    bool eof_;

    //            0    second   front    back   buffer_size
    // to read    [...2...]       [...1...]
    // to write            [..2..]         [...1...]

    std::function<void(void)> continue_write_;
    std::function<void(void)> continue_read_;

    void continue_write(
      const service_provider & sp,
      const std::function<void(const service_provider & sp, size_t len)> & done,
      const item_type * data,
      size_t data_size)
    {
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);
      if (this->back_ < sizeof(this->buffer_)) {
        auto len = sizeof(this->buffer_) - this->back_;
        if (len > data_size) {
          len = data_size;
        }
        
        memcpy(this->buffer_ + this->front_, data, len);
        this->back_ += len;
        
        mt_service::async(sp, [sp, done, len](){
          done(sp, len);
        });
        
        if(this->continue_read_){
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }
      }
      else if (this->second_ < this->front_) {
        auto len = this->front_ - this->second_;
        if (len > data_size) {
          len = data_size;
        }
        memcpy(this->buffer_ + this->second_, data, len);
        this->second_ += len;
        
        mt_service::async(sp, [sp, done, len](){
          done(sp, len);
        });
        
        if(this->continue_read_){
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }
      }
      else {
        this->continue_write_ = std::bind(&async_stream::continue_write, this, sp, done, data, data_size);
      }
    }
    
    void write_all(
      const service_provider & sp,
      const std::function<void(const service_provider & sp)> & done,
      const item_type * data,
      size_t data_size)
    {
      this->continue_write(sp, [this, done, data, data_size](const service_provider & sp, size_t len){
        if(len == data_size){
          done(sp);
        }
        else {
          this->write_all(sp, done, data + len, data_size - len);
        }
      }, data, data_size);
    }

    void continue_read(
      const service_provider & sp,
      const std::function<void(const service_provider & sp, size_t readed)> & done,
      item_type * buffer,
      size_t buffer_size)
    {
      std::unique_lock<std::mutex> lock(this->buffer_mutex_);

      if (this->front_ < this->back_) {
        auto len = this->back_ - this->front_;
        if (len > buffer_size) {
          len = buffer_size;
        }
        memcpy(buffer, this->buffer_ + this->front_, len);
        this->front_ += len;
        
        if(this->front_ == this->back_){
          this->front_ = 0;
          this->back_ = this->second_;
          this->second_ = 0;
        }          
        
        mt_service::async(sp, [sp, done, len](){
          done(sp, len);
        });
        
        if(this->continue_write_){
          std::function<void(void)> f;
          this->continue_write_.swap(f);
          mt_service::async(sp, f);
        }
      }
      else if(this->eof_){
        mt_service::async(sp, [sp, done](){
          done(sp, 0);
        });
      }
      else {
        this->continue_read_ = std::bind(&async_stream::continue_read, this, sp, done, buffer, buffer_size);
      }
    }
  };
  
  template <typename item_type>
  class stream_read
  {
  public:
    stream_read(const std::shared_ptr<async_stream<item_type>> & stream)
    : stream_(stream)
    {
    }
    
    using outgoing_item_type = item_type;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template <typename context_type>
    class handler : public async_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const stream_read & args)
      : base_class(context),
        stream_(args.stream_)
      {
      }
      
      void async_get_data(const service_provider & sp)
      {
        this->continue_get_data(sp);
      }
      
    private:
      std::shared_ptr<async_stream<item_type>> stream_;
      
      void continue_get_data(const service_provider & sp)
      {
        this->stream_->read_async(sp, this->output_buffer_, this->output_buffer_size_)
        .wait(
          [this](const service_provider & sp, size_t readed){
            if(this->processed(sp, readed)){
              this->continue_get_data(sp);
            }
          },
          [this](const service_provider & sp, std::exception_ptr ex){
            this->error(sp, ex);
          },
          sp);          
      }
    };
    
    
  private:
    std::shared_ptr<async_stream<item_type>> stream_;
  };
  
  template <typename item_type>
  class stream_write
  {
  public:
    stream_write(const std::shared_ptr<async_stream<item_type>> & stream)
    : stream_(stream)
    {
    }
    
    using incoming_item_type = item_type;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template <typename context_type>
    class handler : public async_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const stream_write & args)
      : base_class(context),
        stream_(args.stream_)
      {
      }
      
      void async_push_data(const service_provider & sp)
      {
        this->continue_push_data(sp);
      }
      
    private:
      std::shared_ptr<async_stream<item_type>> stream_;
      
      void continue_push_data(const service_provider & sp)
      {
        this->stream_->write_async(sp, this->input_buffer_, this->input_buffer_size_)
        .wait(
          [this](const service_provider & sp, size_t written){
            if(this->processed(sp, written)){
              this->continue_push_data(sp);
            }
          },
          [this](const service_provider & sp, std::exception_ptr ex){
            this->error(sp, ex);
          },
          sp);          
      }
    };
    
    
  private:
    std::shared_ptr<async_stream<item_type>> stream_;
  };
 
}

#endif // __VDS_CORE_ASYNC_STREAM_H_
 