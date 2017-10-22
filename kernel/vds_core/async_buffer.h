#ifndef __VDS_CORE_ASYNC_STREAM_H_
#define __VDS_CORE_ASYNC_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <mutex>
#include <memory>

#include "async_task.h"
#include "mt_service.h"
#include "not_mutex.h"
#include "stream.h"

namespace vds {

  template <typename item_t>
  class continuous_buffer : public stream_async<item_t>
  {
  public:
    using item_type = item_t;

    continuous_buffer()
      : second_(0), front_(0), back_(0), eof_(false)
    {
    }

    ~continuous_buffer()
    {
      if (0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_) {
        if(!std::current_exception()){
          throw std::runtime_error("continuous_buffer::~continuous_buffer logic error");
        }
      }
    }

    async_task<> write_async(
      const service_provider & sp,
      const item_type * data,
      size_t data_size) override      
    {
      imt_service::async_enabled_check(sp);

      if (0 == data_size && this->eof_) {
        throw std::runtime_error("Logic error");
      }
      
      this->in_mutex_.lock();
      this->in_mutex_stack_ = sp.full_name();

      return [this, data, data_size, sp](
          const async_result<> & done) {

        if (0 == data_size) {
          if (this->eof_) {
            this->in_mutex_.unlock();
            throw std::runtime_error("continuous_buffer::write_all_async logic error");
          }
          this->eof_ = true;
          this->eof_stack_ = sp.full_name();

          if (this->continue_read_) {
            std::function<void(void)> f;
            this->continue_read_.swap(f);
#ifdef _DEBUG
            mt_service::async(sp, 
              [f]() {
              f();
            });
#else
            mt_service::async(sp, f);
#endif
          }
          
          this->in_mutex_.unlock();
          mt_service::async(sp, [sp, done]() {
            done();
          });
        }
        else {
          this->in_mutex_.unlock();
          this->write_all(sp, [done](){
            done();
          }, data, data_size);
        }
      };
    }
    
    async_task<> write_value_async(const service_provider & sp, const item_type & data)
    {
      imt_service::async_enabled_check(sp);
      
      auto p = std::make_shared<item_type>(data);

      return this->write_async(sp, p.get(), 1).then([p]() {});
    }

    async_task<size_t /*readed*/> read_async(const service_provider & sp, item_type * buffer, size_t buffer_size)
    {
      imt_service::async_enabled_check(sp);

      if (this->continue_read_) {
        throw std::runtime_error("Logic error 29");
      }
      
      this->out_mutex_.lock();
      this->out_mutex_stack_ = sp.full_name();

      return [this, buffer, buffer_size, sp](const async_result<size_t /*readed*/> & result) {

        this->continue_read(sp, [this, result](size_t readed){
          this->out_mutex_.unlock();
          result(readed);
        }, buffer, buffer_size);
      };
    }

    void reset()
    {
      if (!this->eof_ || 0 != this->second_ || 0 != this->front_ || 0 != this->back_ || this->continue_read_ || this->continue_write_) {
        throw std::runtime_error("continuous_buffer::reset logic error");
      }

      this->eof_ = false;
    }

  private:
    std::string in_mutex_stack_;
    not_mutex in_mutex_;
    std::string out_mutex_stack_;
    not_mutex out_mutex_;
    std::mutex buffer_mutex_;
    item_type buffer_[4096];
    uint32_t second_;
    uint32_t front_;
    uint32_t back_;
    bool eof_;
    std::string eof_stack_;
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
      if (this->back_ < sizeof(this->buffer_) / sizeof(this->buffer_[0])) {
        auto len = sizeof(this->buffer_) / sizeof(this->buffer_[0]) - this->back_;
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

        lock.unlock();
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

        lock.unlock();
        mt_service::async(sp, [sp, done, len]() {
          done(sp, len);
        });
      }
      else {
        this->continue_write_ = std::bind(&continuous_buffer::continue_write, this, sp, done, data, data_size);
      }
    }
    
    void write_all(
      const service_provider & sp,
      const std::function<void()> & done,
      const item_type * data,
      size_t data_size)
    {
      this->continue_write(sp, [this, done, data, data_size](const service_provider & sp, size_t len){
        if(len == data_size){
          done();
        }
        else {
          this->write_all(sp, done, data + len, data_size - len);
        }
      }, data, data_size);
    }

    void continue_read(
      const service_provider & sp,
      const std::function<void(size_t readed)> & done,
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
          done(len);
        });
      }
      else if(this->eof_){

        lock.unlock();
        mt_service::async(sp, [sp, done](){
          done(0);
        });
      }
      else {
        this->continue_read_ = std::bind(&continuous_buffer::continue_read, this, sp, done, buffer, buffer_size);
      }
    }
  };

  template <typename item_t>
  class async_buffer
  {
  public:
    using item_type = item_t;
    async_buffer()
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
        [this](
          const async_result<size_t> & result,
          size_t readed) {
          this->data_mutex_.lock();
          this->ready_to_data_ = true;
          this->data_barier_.notify_one();
          this->data_mutex_.unlock();
          
          result(readed);
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
    continuous_buffer<item_type> data_;
  };
  
  template <typename item_type, typename source_type, typename target_type>
  class _continue_copy_stream : public std::enable_shared_from_this<_continue_copy_stream<item_type, source_type, target_type>>
  {
  public:
    _continue_copy_stream(
      const service_provider & sp,
      const std::shared_ptr<source_type> & source,
      const std::shared_ptr<target_type> & target)
    : sp_(sp), source_(source), target_(target)
    {
    }
    
    static async_task<> _continue_copy(
      const std::shared_ptr<_continue_copy_stream> & context)
    {
      return context->source_->read_async(
        context->sp_,
        context->buffer_,
        sizeof(context->buffer_) / sizeof(context->buffer_[0])).then(
        [context](size_t readed){
          return context->target_->write_async(context->sp_, context->buffer_, readed).
            then([context, is_eof = (0 == readed)](){
              if(is_eof){
                return async_task<>::empty();
              }
              else {
                return _continue_copy(context);
              }              
            });
        }
      );    
    }
        
  private:
    service_provider sp_;
    std::shared_ptr<source_type> source_;
    std::shared_ptr<target_type> target_;
    item_type buffer_[1024];
  };
  
  
  template <typename item_type>
  inline async_task<> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<async_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, continuous_buffer<item_type>, async_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, continuous_buffer<item_type>, async_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline async_task<> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<async_buffer<item_type>> & source,
    const std::shared_ptr<continuous_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, async_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, async_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline async_task<> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<continuous_buffer<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, continuous_buffer<item_type>, continuous_buffer<item_type>>::_continue_copy(context);
  }
  
  template <typename item_type>
  inline async_task<> copy_stream(
    const service_provider & sp,
    const std::shared_ptr<continuous_buffer<item_type>> & source,
    const std::shared_ptr<stream_async<item_type>> & target)
  {
    auto context = std::make_shared<_continue_copy_stream<item_type, continuous_buffer<item_type>, stream_async<item_type>>>(sp, source, target);
    return _continue_copy_stream<item_type, continuous_buffer<item_type>, stream_async<item_type>>::_continue_copy(context);
  }
}

#endif // __VDS_CORE_ASYNC_STREAM_H_
 
