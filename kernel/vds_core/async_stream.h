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
#include "not_mutex.h"

namespace vds {

  template <typename item_t>
  class continuous_stream
  {
  public:
    using item_type = item_t;

    continuous_stream()
      : second_(0), front_(0), back_(0), eof_(false)
    {
    }

    async_task<> write_all_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      imt_service::async_enabled_check(sp);

      if (0 == data_size && this->eof_) {
        throw std::runtime_error("Logiñ error");
      }
      
      this->in_mutex_.lock();

      return create_async_task(
        [this, data, data_size](
          const std::function<void(const service_provider & sp)> & done,
          const error_handler & on_error,
          const service_provider & sp) {

        if (0 == data_size) {
          if (this->eof_) {
            this->in_mutex_.unlock();
            throw std::runtime_error("Logiñ error");
          }
          this->eof_ = true;

          if (this->continue_read_) {
            std::function<void(void)> f;
            this->continue_read_.swap(f);
            mt_service::async(sp, f);
          }
          
          mt_service::async(sp, [this, sp, done]() {
            this->in_mutex_.unlock();
            done(sp);
          });
        }
        else {
          this->write_all(sp, [this, done](const service_provider & sp){
            this->in_mutex_.unlock();
            done(sp);
          }, data, data_size);
        }
      });
    }
    
    async_task<> write_value_async(const service_provider & sp, const item_type & data)
    {
      imt_service::async_enabled_check(sp);
      
      auto p = std::make_shared<item_type>(data);

      return create_async_task([this, p](
        const std::function<void(const service_provider & sp)> & done,
        const error_handler & on_error,
        const service_provider & sp){
        this->write_all_async(sp, p.get(), 1).wait(
          done,
          on_error,
          sp);
      });
    }
    
    async_task<size_t> write_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      imt_service::async_enabled_check(sp);

      if (this->continue_write_) {
        throw std::runtime_error("Login error");
      }
      
      this->in_mutex_.lock();

      return create_async_task(
        [this, data, data_size](
          const std::function<void(const service_provider & sp, size_t written)> & done,
          const error_handler & on_error,
          const service_provider & sp) {

          if(0 == data_size) {
            if (this->eof_) {
              throw std::runtime_error("Login error");
            }
            this->eof_ = true;

            if(this->continue_read_){
              std::function<void(void)> f;
              this->continue_read_.swap(f);
              mt_service::async(sp, f);
            }
            
            mt_service::async(sp, [this, sp, done]() {
              this->in_mutex_.unlock();
              done(sp, 0);
            });
          } else {
            this->continue_write(sp, [this, done](const service_provider & sp, size_t written){
              this->in_mutex_.unlock();
              done(sp, written);
            }, data, data_size);
          }
      });
    }

    async_task<size_t /*readed*/> read_async(const service_provider & sp, item_type * buffer, size_t buffer_size)
    {
      imt_service::async_enabled_check(sp);

      if (this->continue_read_) {
        throw std::runtime_error("Logic error 29");
      }
      
      this->out_mutex_.lock();

      return create_async_task(
        [this, buffer, buffer_size](
          const std::function<void(const service_provider & sp, size_t readed)> & done,
          const error_handler & on_error,
          const service_provider & sp) {

        this->continue_read(sp, [this, done](const service_provider & sp, size_t readed){
          this->out_mutex_.unlock();
          done(sp, readed);
        }, buffer, buffer_size);
      });
    }

    void reset()
    {
      if (!this->eof_ || 0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_) {
        throw std::runtime_error("Logiñ error");
      }

      this->eof_ = false;
    }

  private:
    not_mutex in_mutex_;
    not_mutex out_mutex_;
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
        
        std::copy(data, data + len, this->buffer_ + this->back_);
        this->back_ += len;
       
        if(this->continue_read_){
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }

        mt_service::async(sp, [sp, done, len]() {
          done(sp, len);
        });
      }
      else if (this->second_ < this->front_) {
        auto len = this->front_ - this->second_;
        if (len > data_size) {
          len = data_size;
        }
        std::copy(data, data + len, this->buffer_ + this->second_);
        this->second_ += len;
       
        if(this->continue_read_){
          std::function<void(void)> f;
          this->continue_read_.swap(f);
          mt_service::async(sp, f);
        }

        mt_service::async(sp, [sp, done, len]() {
          done(sp, len);
        });
      }
      else {
        this->continue_write_ = std::bind(&continuous_stream::continue_write, this, sp, done, data, data_size);
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
        std::copy(this->buffer_ + this->front_, this->buffer_ + this->front_ + len, buffer);
        this->front_ += len;
        
        if(this->front_ == this->back_){
          this->front_ = 0;
          this->back_ = this->second_;
          this->second_ = 0;
        }          
        
        if(this->continue_write_){
          std::function<void(void)> f;
          this->continue_write_.swap(f);
          mt_service::async(sp, f);
        }

        mt_service::async(sp, [sp, done, len]() {
          done(sp, len);
        });
      }
      else if(this->eof_){
        mt_service::async(sp, [sp, done](){
          done(sp, 0);
        });
      }
      else {
        this->continue_read_ = std::bind(&continuous_stream::continue_read, this, sp, done, buffer, buffer_size);
      }
    }
  };

  template <typename item_t>
  class async_stream
  {
  public:
    using item_type = item_t;
    async_stream()
      : ready_to_data_(true)
    {
    }

    async_task<> write_value_async(const service_provider & sp, const item_type & data)
    {
      std::unique_lock<std::mutex> lock(this->data_mutex_);
      while (!this->ready_to_data_) {
        this->data_barier_.wait(lock);
      }

      this->ready_to_data_ = false;
      return this->data_.write_value_async(sp, data).then(
        [this](const std::function<void(const service_provider & sp)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp) {
          this->data_mutex_.lock();
          this->ready_to_data_ = true;
          this->data_barier_.notify_one();
          this->data_mutex_.unlock();
          
          done(sp);
        });
    }

    async_task<size_t> write_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      std::unique_lock<std::mutex> lock(this->data_mutex_);
      while (!this->ready_to_data_) {
        this->data_barier_.wait_for(lock, std::chrono::microseconds(100));
      }

      this->ready_to_data_ = false;

      return this->data_.write_async(sp, data, data_size).then(
        [this](const std::function<void(const service_provider & sp, size_t)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp,
          size_t readed) {
          this->data_mutex_.lock();
          this->ready_to_data_ = true;
          this->data_barier_.notify_one();
          this->data_mutex_.unlock();
          
          done(sp, readed);
        });
    }
    
    async_task<> write_all_async(const service_provider & sp, const item_type * data, size_t data_size)
    {
      std::unique_lock<std::mutex> lock(this->data_mutex_);
      while (!this->ready_to_data_) {
        this->data_barier_.wait_for(lock, std::chrono::microseconds(100));
      }

      this->ready_to_data_ = false;

      return this->data_.write_all_async(sp, data, data_size).then(
        [this](const std::function<void(const service_provider & sp)> & done,
          const vds::error_handler & on_error,
          const vds::service_provider & sp) {
          this->data_mutex_.lock();
          this->ready_to_data_ = true;
          this->data_barier_.notify_one();
          this->data_mutex_.unlock();
          
          done(sp);
        });
    }


    async_task<size_t /*readed*/> read_async(const service_provider & sp, item_type * buffer, size_t buffer_size)
    {
      return this->data_.read_async(sp, buffer, buffer_size);
    }
  private:
    bool ready_to_data_;
    std::condition_variable data_barier_;
    std::mutex data_mutex_;
    continuous_stream<item_type> data_;
  };

  template <typename stream_type>
  class stream_read
  {
  public:
    stream_read(const std::shared_ptr<stream_type> & stream)
    : stream_(stream)
    {
    }

    typedef typename stream_type::item_type outgoing_item_type;
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
      std::shared_ptr<stream_type> stream_;
      
      void continue_get_data(const service_provider & sp)
      {
        this->stream_->read_async(sp, this->output_buffer(), this->output_buffer_size())
        .wait(
          [this](const service_provider & sp, size_t readed){
            if(this->processed(sp, readed)){
              this->continue_get_data(sp);
            }
          },
          [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
            this->error(sp, ex);
          },
          sp);          
      }
    };
    
    
  private:
    std::shared_ptr<stream_type> stream_;
  };
  
  template <typename stream_type>
  class stream_write
  {
  public:
    stream_write(const std::shared_ptr<stream_type> & stream)
    : stream_(stream)
    {
    }
    
    typedef typename stream_type::item_type incoming_item_type;
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
      std::shared_ptr<stream_type> stream_;
      
      void continue_push_data(const service_provider & sp)
      {
        this->stream_->write_async(sp, this->input_buffer(), this->input_buffer_size())
        .wait(
          [this](const service_provider & sp, size_t written){
            if(this->processed(sp, written)){
              this->continue_push_data(sp);
            }
          },
          [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
            this->error(sp, ex);
          },
          sp);          
      }
    };
    
    
  private:
    std::shared_ptr<stream_type> stream_;
  };
 
}

#endif // __VDS_CORE_ASYNC_STREAM_H_
 