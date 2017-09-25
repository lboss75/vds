#ifndef __VDS_CORE_DATAFLOW_H_
#define __VDS_CORE_DATAFLOW_H_

#include <set>
#include "shutdown_event.h"
#include "service_provider.h"
#include "async_task.h"
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  
  template <typename context_type, typename implementation_type>
  class sync_dataflow_source
  {
  public:
    using common_data_type = typename context_type::common_data_type;
    using outgoing_item_type = typename context_type::outgoing_item_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;
    
    sync_dataflow_source(const context_type & context)
    : target_(context.target_), common_data_(context.common_data_)
    {
    }
    
    sync_dataflow_source(const sync_dataflow_source &) = delete;
    sync_dataflow_source(sync_dataflow_source&&) = delete;
    sync_dataflow_source & operator = (const sync_dataflow_source &) = delete;
    sync_dataflow_source & operator = (sync_dataflow_source&&) = delete;
    
    bool get_data(
      const service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;
      readed = static_cast<implementation_type *>(this)->sync_get_data(sp);
      if (0 == readed) {
        if (this->common_data_->step_finish(sp, context_type::INDEX)) {
          return false;
        }
      }
      return true;
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    outgoing_queue_type * target_;
    common_data_type * common_data_;
    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;
    
  protected:
    outgoing_item_type * output_buffer() const { return this->output_buffer_; }
    size_t output_buffer_size() { return this->output_buffer_size_; }
    outgoing_item_type & output_buffer(size_t index) const { return this->output_buffer_[index]; }
    
    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };

  template <typename context_type, typename implementation_type>
  class async_dataflow_source
  {
  public:
    using common_data_type = typename context_type::common_data_type;
    using outgoing_item_type = typename context_type::outgoing_item_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;

    async_dataflow_source(const context_type & context)
      : target_(context.target_), common_data_(context.common_data_)
    {
    }

    async_dataflow_source(const async_dataflow_source &) = delete;
    async_dataflow_source(async_dataflow_source&&) = delete;
    async_dataflow_source & operator = (const async_dataflow_source &) = delete;
    async_dataflow_source & operator = (async_dataflow_source&&) = delete;

    bool get_data(
      const service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;
      static_cast<implementation_type *>(this)->async_get_data(sp);
      return false;
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    outgoing_queue_type * target_;
    common_data_type * common_data_;
    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;
    
  protected:
    outgoing_item_type * output_buffer() const { return this->output_buffer_; }
    size_t output_buffer_size() { return this->output_buffer_size_; }
    outgoing_item_type & output_buffer(size_t index) const { return this->output_buffer_[index]; }
    
    bool processed(
      const service_provider & sp,
      size_t written)
    {
      if(0 == written){
        if (this->common_data_->step_finish(sp, context_type::INDEX)) {
          return false;
        }
      }
      
      return this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_);
    }
    
    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };
  
  template <typename context_type, typename implementation_type>
  class sync_dataflow_filter
  {
  public:
    using incoming_item_type = typename context_type::incoming_item_type;
    using outgoing_item_type = typename context_type::outgoing_item_type;

    using incoming_queue_type = typename context_type::incoming_queue_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;

    sync_dataflow_filter(const context_type & context)
      : source_(context.source_),
      target_(context.target_),
      common_data_(context.common_data_),
      input_buffer_(nullptr),
      output_buffer_(nullptr)
    {
    }

    bool get_data(
      const service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & written)
    {
      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;

      if (nullptr == this->input_buffer_) {
        if (!this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_)) {
          return false;
        }

        if (0 == this->input_buffer_size_) {
          written = 0;
          return true;
        }
      }

      for (;;) {

        if (0 == this->input_buffer_size_) {
          if (!this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_)) {
            return false;
          }
        }

        size_t readed;
        try {
          static_cast<implementation_type *>(this)->sync_process_data(sp, readed, written);
        }
        catch (const std::exception & ex) {
          this->common_data_->step_error(sp, context_type::INDEX, std::make_shared<std::exception>(ex));
          return false;
        }
        catch (...) {
          this->common_data_->step_error(sp, context_type::INDEX, std::make_shared<std::runtime_error>("Unhandled error"));
          return false;
        }
        
        if(0 < readed){
          if (readed > this->input_buffer_size_) {
            throw std::runtime_error("Invalid logic");
          }
          
          this->input_buffer_ += readed;
          this->input_buffer_size_ -= readed;
          this->source_->readed(sp, readed);
        }

        if (0 < written) {
          return true;
        }
        else {
          if (0 == readed) {
            if (0 != this->input_buffer_size_) {
              throw std::runtime_error("Invalid logic");
            }

            if (this->common_data_->step_finish(sp, context_type::INDEX)) {
              return false;
            }
            
            return true;
          }
        }
      }
    }

    bool push_data(
      const service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;

      size_t written;
      static_cast<implementation_type *>(this)->sync_process_data(sp, readed, written);

      if (0 < written) {
        if (readed > this->input_buffer_size_) {
          throw std::runtime_error("Invalid login");
        }
      }
      else {
        if (0 == readed) {
          if (0 != this->input_buffer_size_) {
            throw std::runtime_error("Invalid login");
          }

          if (this->common_data_->step_finish(sp, context_type::INDEX)) {
            return false;
          }
        }
      }

      if (0 < written || 0 == readed) {
        if (!this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
          return false;
        }
      }
      
      return true;
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    incoming_queue_type * source_;
    outgoing_queue_type * target_;
    typename context_type::common_data_type * common_data_;

    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;

    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;
  protected:
    incoming_item_type * input_buffer() const { return this->input_buffer_; }
    size_t input_buffer_size() const { return this->input_buffer_size_; }
    incoming_item_type & input_buffer(size_t index) const { return this->input_buffer_[index];}
    
    outgoing_item_type * output_buffer() const { return this->output_buffer_; }
    size_t output_buffer_size() { return this->output_buffer_size_; }
    outgoing_item_type & output_buffer(size_t index) const { return this->output_buffer_[index]; }

    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };

  
  template <typename context_type, typename implementation_type>
  class async_dataflow_filter
  {
  public:

    using incoming_item_type = typename context_type::incoming_item_type;
    using outgoing_item_type = typename context_type::outgoing_item_type;

    using incoming_queue_type = typename context_type::incoming_queue_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;

    async_dataflow_filter(const context_type & context)
      : source_(context.source_),
      target_(context.target_),
      common_data_(context.common_data_),
      input_buffer_(nullptr)
    {
    }

    //virtual void async_process_data(const service_provider & sp) = 0;

    bool get_data(
      const service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;

      if (nullptr == this->input_buffer_) {
        this->readed_ = 0;
        if (!this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_)) {
          return false;
        }
      }

      if (0 == this->input_buffer_size_) {
        readed = 0;
        return true;
      }

      static_cast<implementation_type *>(this)->async_process_data(sp);
      return false;
    }

    bool push_data(
      const service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & written)
    {
      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;
      this->readed_ = 0;

      static_cast<implementation_type *>(this)->async_process_data(sp);

      return false;
    }
    
    bool processed(const service_provider & sp, size_t readed, size_t written)
    {
      if(0 == written && 0 == readed) {
        if (0 != this->input_buffer_size_) {
          throw std::runtime_error("Logic error 13");
        }
        
        if (this->common_data_->step_finish(sp, context_type::INDEX)) {
          return false;
        }

        if (!this->target_->push_data(sp, 0, this->output_buffer_, this->output_buffer_size_)) {
          return false;
        }

        return true;
      }
      else if (0 == written) {
        if (this->input_buffer_size_ < readed) {
          throw std::runtime_error("Login error");
        }
        
        this->source_->readed(sp, readed);

        this->input_buffer_ += readed;
        this->input_buffer_size_ -= readed;
        
        return this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_);
      }
      else {
        if (this->input_buffer_size_ < readed) {
          throw std::runtime_error("Login error");
        }
          
        this->source_->readed(sp, readed);

        this->input_buffer_ += readed;
        this->input_buffer_size_ -= readed;
      }
        
      if (!this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
        return false;
      }
        
      return this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_);
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    incoming_queue_type * source_;
    outgoing_queue_type * target_;
    typename context_type::common_data_type * common_data_;

    size_t readed_;

    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;

    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;

  protected:
    incoming_item_type * input_buffer() const { return this->input_buffer_; }
    size_t input_buffer_size() const { return this->input_buffer_size_; }
    incoming_item_type & input_buffer(size_t index) const { return this->input_buffer_[index];}
    
    outgoing_item_type * output_buffer() const { return this->output_buffer_; }
    size_t output_buffer_size() { return this->output_buffer_size_; }
    outgoing_item_type & output_buffer(size_t index) const { return this->output_buffer_[index]; }
    
    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };
  
  template <typename context_type, typename implementation_type>
  class sync_dataflow_target
  {
  public:
    using common_data_type = typename context_type::common_data_type;
    using incoming_item_type = typename context_type::incoming_item_type;
    using incoming_queue_type = typename context_type::incoming_queue_type;
    sync_dataflow_target(const context_type & context)
      : source_(context.source_),
      common_data_(context.common_data_),
      input_buffer_(nullptr)
    {
    }
    
    sync_dataflow_target(const sync_dataflow_target &) = delete;
    sync_dataflow_target(sync_dataflow_target&&) = delete;
    sync_dataflow_target & operator = (const sync_dataflow_target &) = delete;
    sync_dataflow_target & operator = (sync_dataflow_target&&) = delete;
    
    bool push_data(
      const service_provider & sp,
      incoming_item_type * values,
      size_t count,
      size_t & written)
    {
      this->input_buffer_ = values;
      this->input_buffer_size_ = count;

      written = static_cast<implementation_type *>(this)->sync_push_data(sp);
      if (0 == written && 0 == count) {
        return !this->common_data_->step_finish(sp, context_type::INDEX);
      }
      
      return true;
    }

    void start(const service_provider & sp)
    {
      for(;;) {
        if (!this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_)) {
          return;
        }
        
        auto readed = static_cast<implementation_type *>(this)->sync_push_data(sp);

        if (0 == this->input_buffer_size_ && 0 == readed) {
          this->common_data_->step_finish(sp, context_type::INDEX);
          break;
        }
        
        if(0 < readed){
          this->source_->readed(sp, readed);
        }
      }
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    incoming_queue_type * source_;
    common_data_type * common_data_;

    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;

  protected:
    incoming_item_type * input_buffer() const { return this->input_buffer_; }
    size_t input_buffer_size() const { return this->input_buffer_size_; }
    incoming_item_type & input_buffer(size_t index) const { return this->input_buffer_[index];}
    
    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };

  template <typename context_type, typename implementation_type>
  class async_dataflow_target
  {
  public:
    using common_data_type = typename context_type::common_data_type;
    using incoming_queue_type = typename context_type::incoming_queue_type;
    using incoming_item_type = typename context_type::incoming_item_type;
    async_dataflow_target(const context_type & context)
    : source_(context.source_),
      common_data_(context.common_data_),
      input_buffer_(nullptr)
    {
    }

    async_dataflow_target(const async_dataflow_target &) = delete;
    async_dataflow_target(async_dataflow_target&&) = delete;
    async_dataflow_target & operator = (const async_dataflow_target &) = delete;
    async_dataflow_target & operator = (async_dataflow_target&&) = delete;

    bool push_data(
      const service_provider & sp,
      incoming_item_type * values,
      size_t count,
      size_t & written)
    {
      this->input_buffer_ = values;
      this->input_buffer_size_ = count;

      static_cast<implementation_type *>(this)->async_push_data(sp);
      return false;
    }

    void start(const service_provider & sp)
    {
      if (nullptr == this->input_buffer_) {
        if (!this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_)) {
          return;
        }
      }

      static_cast<implementation_type *>(this)->async_push_data(sp);
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    incoming_queue_type * source_;
    common_data_type * common_data_;
    
    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;
    
  protected:
    incoming_item_type * input_buffer() const { return this->input_buffer_; }
    size_t input_buffer_size() const { return this->input_buffer_size_; }
    incoming_item_type & input_buffer(size_t index) const { return this->input_buffer_[index];}
    
    bool processed(const service_provider & sp, size_t written)
    {
      if (0 == written) {
        this->common_data_->step_finish(sp, context_type::INDEX);
        return false;
      }
      
      this->source_->readed(sp, written);
      
      return this->source_->get_data(sp, this->input_buffer_, this->input_buffer_size_);
    }
    
    void error(const service_provider & sp, const std::shared_ptr<std::exception> & ex)
    {
      this->common_data_->step_error(sp, context_type::INDEX, ex);
    }
  };

  template <typename... functor_types>
  class _dataflow
  {
    using tuple_type = std::tuple<functor_types...>;
  public:
    
    static async_task<> create_task(functor_types... functors)
    {
      return create_async_task(
        [builder = tuple_type(std::forward<functor_types>(functors)...)](
          const std::function<void(const service_provider & sp)> & done,
          const error_handler & on_error,
          const service_provider & sp) {
        auto p = std::make_shared<starter>(done, on_error, builder);
        p->start(p, sp);
      });
    }        
    
  private:
    class final_step_type;
    class starter;
    
    template<std::size_t index>
    using _functor_type_t = typename std::remove_reference<typename std::tuple_element<index, tuple_type>::type>::type;
    
    class common_data
    {
    public:
      virtual bool step_finish(const service_provider & sp, size_t index) = 0;
      virtual bool step_error(const service_provider & sp, size_t index, const std::shared_ptr<std::exception> & error) = 0;
    };
    
    template<
      std::size_t index,
      typename enabled = void,
      bool dummy = true>
    class context;
      
    template<std::size_t index, bool dummy = false>
    class step_type
    : public _functor_type_t<index>::template handler<context<index>>
    {
    public:
      using context_t = context<index>;
      using incoming_item_type = typename _functor_type_t<index>::incoming_item_type;
      using outgoing_item_type = typename _functor_type_t<index>::outgoing_item_type;
      using incoming_queue_type = typename context_t::incoming_queue_type;
      using outgoing_queue_type = typename context_t::outgoing_queue_type;
      step_type(
        const context_t & ctx,
        const _functor_type_t<index> & args)
      : _functor_type_t<index>::template handler<context<index>>(ctx, args)
      {
      }
      
    };
    
    template<bool dummy>
    class step_type<0, dummy>
    : public _functor_type_t<0>::template handler<context<0>>
    {
    public:
      using context_t = context<0>;
      using outgoing_item_type = typename _functor_type_t<0>::outgoing_item_type;
      using outgoing_queue_type = typename context_t::outgoing_queue_type;
      step_type(const context_t & ctx, const _functor_type_t<0> & args)
      : _functor_type_t<0>::template handler<context<0>>(ctx, args)
      {
      }
      
    };

    template <std::size_t index>
    class queue_stream;
    
    template<
      std::size_t index,
      typename enabled,
      bool dummy>
    class context
    {
    public:
      static constexpr std::size_t INDEX = index;
      using common_data_type = common_data;
      using incoming_item_type = typename _functor_type_t<index>::incoming_item_type;
      using outgoing_item_type = typename _functor_type_t<index>::outgoing_item_type;
      using incoming_queue_type = queue_stream<index - 1>;
      using outgoing_queue_type = queue_stream<index>;
      
      context(
        common_data * data,
        incoming_queue_type * source,
        outgoing_queue_type * target)
      : common_data_(data),
        source_(source), target_(target)
      {
      }
      
      common_data * common_data_;
      incoming_queue_type * source_;
      outgoing_queue_type * target_;
    };
    
    template<
      typename enabled,
      bool dummy>
    class context<0, enabled, dummy>
    {
    public:
      static constexpr std::size_t INDEX = 0;
      using common_data_type = common_data;
      using outgoing_item_type = typename _functor_type_t<0>::outgoing_item_type;
      using outgoing_queue_type = queue_stream<0>;
      
      context(common_data * data, outgoing_queue_type * target)
      : common_data_(data), target_(target)
      {
      }
      
      common_data * common_data_;
      outgoing_queue_type * target_;
    };
    
    template <std::size_t index>
    class queue_stream
    {
    public:
      using incoming_item_type = typename _functor_type_t<index>::outgoing_item_type;
      using outgoing_item_type = typename _functor_type_t<index + 1>::incoming_item_type;

      using data_source = step_type<index>;
      using data_target = typename std::conditional<index < std::tuple_size<tuple_type>::value - 2, step_type<index + 1>, final_step_type>::type;
      queue_stream(
        common_data * data,
        data_source * source,
        data_target * target)
      : source_(source), target_(target),
        common_data_(data),
        second_(0),
        front_(0),
        back_(0)
      {
      }
      
      //from source
      bool push_data(
        const service_provider & sp,
        size_t count,
        incoming_item_type *& buffer,
        size_t & buffer_len)
      {
        try {
          if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
            this->back_ += count;
          }
          else {
            if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
              this->second_ += count;
            }
            else {
              throw std::runtime_error("Invalid logic");
            }
          }

          if (this->front_ == this->back_) {
            this->front_ = 0;
            this->back_ = this->second_;
            this->second_ = 0;
          }

          incoming_item_type * read_buffer;
          size_t read_len;
          if(this->get_read_buffer(read_buffer, read_len)){
            size_t readed;
            if (!this->target_->push_data(sp, read_buffer, read_len, readed)) {
              return false;
            }
              
            this->front_ += readed;
              
            if (this->front_ == this->back_) {
              this->front_ = 0;
              this->back_ = this->second_;
              this->second_ = 0;
            }
          }
          else if (0 == count) {
            size_t readed;
            return this->target_->push_data(sp, nullptr, 0, readed);
          }
          
          if (!this->get_write_buffer(buffer, buffer_len)) {
            return false;
          }

          return true;
        }
        catch (const std::exception & ex) {
          this->common_data_->step_error(sp, index, std::make_shared<std::exception>(ex));
          return false;
        }
        catch(...){
          this->common_data_->step_error(sp, index, std::make_shared<std::runtime_error>("Unexpected error"));
          return false;
        }
      }
      
      void readed(
        const service_provider & sp,
        size_t readed)
      {
        this->front_ += readed;

        if (this->front_ == this->back_) {
          this->front_ = 0;
          this->back_ = this->second_;
          this->second_ = 0;
        }
      }
      
      bool get_data(
        const service_provider & sp,
        outgoing_item_type *& buffer,
        size_t & buffer_len)
      {
        if (this->get_read_buffer(buffer, buffer_len)) {
          return true;
        }
        
        if (!this->get_write_buffer(buffer, buffer_len)) {
          throw std::runtime_error("Logic error 21");
        }

        size_t readed;
        if(!this->source_->get_data(sp, buffer, buffer_len, readed)){
          return false;
        }
        
        if (0 == readed) {
          buffer_len = 0;
          return true;
        }

        if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
          this->back_ += readed;
        }
        else if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
          this->second_ += readed;
        }
        else {
          throw std::runtime_error("Logic error 21");
        }

        if (!this->get_read_buffer(buffer, buffer_len)) {
          throw std::runtime_error("Logic error 22");
        }
        
        return true;
      }
      
    private:
      data_source * source_;
      data_target * target_;
      common_data * common_data_;
      
      static constexpr size_t BUFFER_SIZE = 
      (_functor_type_t<index>::BUFFER_SIZE > _functor_type_t<index + 1>::BUFFER_SIZE)
      ? _functor_type_t<index>::BUFFER_SIZE : _functor_type_t<index + 1>::BUFFER_SIZE;
      static constexpr size_t MIN_BUFFER_SIZE = (_functor_type_t<index>::MIN_BUFFER_SIZE > _functor_type_t<index + 1>::MIN_BUFFER_SIZE)
      ? _functor_type_t<index>::MIN_BUFFER_SIZE : _functor_type_t<index + 1>::MIN_BUFFER_SIZE;
      
      incoming_item_type buffer_[BUFFER_SIZE];
      uint32_t second_;
      uint32_t front_;
      uint32_t back_;

      //            0    second   front    back   buffer_size
      // to read    [...2...]       [...1...]
      // to write            [..2..]         [...1...]

      bool get_write_buffer(incoming_item_type *& result_buffer, size_t & result_len)
      {
        static_assert(MIN_BUFFER_SIZE < BUFFER_SIZE, "Buffer size is invalid");
        
        if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
          result_buffer = this->buffer_ + this->back_;
          result_len = BUFFER_SIZE - this->back_;
          return true;
        }

        if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
          result_buffer = this->buffer_ + this->second_;
          result_len = this->front_ - this->second_;
          return true;
        }

        return false;
      }

      bool get_read_buffer(incoming_item_type *& result_buffer, size_t & result_len)
      {
        if (this->front_ < this->back_) {
          result_buffer = this->buffer_ + this->front_;
          result_len = this->back_ - this->front_;
          return true;
        }

        return false;
      }
    };
   
    
    template <size_t index, bool dummy = false>
    class pipeline : public pipeline<index - 1, dummy>
    {
      using base_class = pipeline<index - 1, dummy>;
    public:
      pipeline(
        common_data * data,
        queue_stream<index> * queue,
        const tuple_type & args)
      : base_class(data, &queue_, args),
        queue_(data, base_class::step(), &step_),
        step_(context<index>(data, &queue_, queue), std::get<index>(args))
      {
      }
      
      step_type<index> * step(){
        return &this->step_;
      }

      void cancel(const service_provider & sp)
      {
        base_class::cancel(sp);
        this->step_.cancel(sp);
      }
     
    private:
      queue_stream<index - 1> queue_;
      step_type<index> step_;
    };
    
    template <bool dummy>
    class pipeline<0, dummy>
    {
    public:
      pipeline(
        common_data * data,
        queue_stream<0> * queue,
        const tuple_type & args)
      : step_(context<0>(data, queue), std::get<0>(args))
      {
      }
      
      step_type<0> * step(){
        return &this->step_;
      }

      void cancel(const service_provider & sp)
      {
        this->step_.cancel(sp);
      }

    private:
      step_type<0> step_;
    };
    
    class final_context
    {
    public:
      static constexpr std::size_t INDEX = std::tuple_size<tuple_type>::value - 1;
      using common_data_type = common_data;
      using incoming_item_type = typename _functor_type_t<std::tuple_size<tuple_type>::value - 1>::incoming_item_type;
      using incoming_queue_type = queue_stream<std::tuple_size<tuple_type>::value - 2>;
      using terminator_type = starter;
      
      final_context(
        common_data * data,
        incoming_queue_type * source,
        starter * owner)
      : source_(source),
        terminator_(owner),
        common_data_(data)
      {
      }
      
      incoming_queue_type * source_;
      starter * terminator_;
      common_data * common_data_;
    };
    
    class final_step_type
    : public _functor_type_t<std::tuple_size<tuple_type>::value - 1>::template handler<final_context>
    {
      using base_class = typename _functor_type_t<std::tuple_size<tuple_type>::value - 1>::template handler<final_context>;
    public:
      
      using incoming_item_type = typename _functor_type_t<std::tuple_size<tuple_type>::value - 1>::incoming_item_type;
      using incoming_queue_type = typename final_context::incoming_queue_type;
      
      final_step_type(
        const final_context & ctx,
        const _functor_type_t<std::tuple_size<tuple_type>::value - 1> & args)
      : base_class(ctx, args),
        source_(ctx.source_),
        common_data_(ctx.common_data_)
      {
      }
      
    private:
      incoming_queue_type * source_;
      common_data * common_data_;
    };
    
    class starter
    : public common_data,
      public pipeline<std::tuple_size<tuple_type>::value - 2>
    {
      using base_class = pipeline<std::tuple_size<tuple_type>::value - 2>;
    public:
      starter(
        const std::function<void(const service_provider & sp)> & done,
        const error_handler & on_error,
        const tuple_type & args)
      : base_class(this, &queue_, args),
        done_(done), on_error_(on_error),
        step_(final_context(this, &queue_, this), std::get<std::tuple_size<tuple_type>::value - 1>(args)),
        queue_(this, base_class::step(), &step_)
      {
      }
      

      void start(const std::shared_ptr<starter> pthis, const service_provider & sp)
      {
        this->pthis_ = pthis;
        this->step_.start(sp);
      }

      bool step_finish(const service_provider & sp, size_t index) override
      {
        this->done_steps_.emplace(index);
        return this->try_finish(sp);
      }

      bool step_error(const service_provider & sp, size_t index, const std::shared_ptr<std::exception> & error) override
      {
        if (!this->error_) {
          this->error_ = error;
        }

        this->done_steps_.emplace(index);
        return this->try_finish(sp);
      }


    private:
      std::function<void(const service_provider & sp)> done_;
      error_handler on_error_;
      final_step_type step_;
      queue_stream<std::tuple_size<tuple_type>::value - 2> queue_;
      std::shared_ptr<starter> pthis_;

      std::set<size_t> done_steps_;
      std::shared_ptr<std::exception> error_;

      bool try_finish(const service_provider & sp)
      {
        if (std::tuple_size<tuple_type>::value > this->done_steps_.size()) {
          return false;
        }

        if (!this->error_) {
          this->done_(sp);
        }
        else {
          this->on_error_(sp, this->error_);
        }

        this->pthis_.reset();
        return true;
      }
    };
  };
  
  template <typename... functor_types>
  inline async_task<> dataflow(functor_types&&... functors)
  {
    return _dataflow<functor_types...>::create_task(std::forward<functor_types>(functors)...);
  }
  
  template<typename item_type>
  class dataflow_require_once
  {
  public:
    dataflow_require_once(item_type * result)
      : result_(result)
    {
    }

    using incoming_item_type = item_type;
    static constexpr size_t BUFFER_SIZE = 1;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public vds::sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const dataflow_require_once & owner
      )
        : base_class(context),
        completed_(false),
        result_(owner.result_)
      {
      }

      size_t sync_push_data(
        const vds::service_provider & sp)
      {
        if (!this->completed_) {
          if (1 < this->input_buffer_size()) {
            this->error(sp, std::make_shared<std::runtime_error>("Require only one item"));
            return 0;
          }
          else if(1 == this->input_buffer_size()){
            this->completed_ = true;
            *this->result_ = *this->input_buffer();
          }
          else {
            this->error(sp, std::make_shared<std::runtime_error>("Require one item"));
            return 0;
          }
        }
        else if (0 < this->input_buffer_size()) {
          this->error(sp, std::make_shared<std::runtime_error>("Require only one item"));
          return 0;
        }

        return this->input_buffer_size();
      }

    private:
      bool completed_;
      item_type * result_;
    };

  private:
    item_type * result_;
  };
  
  template<typename item_type>
  class dataflow_arguments
  {
  public:
    dataflow_arguments(const item_type * data, size_t count)
    : data_(data), count_(count)
    {
    }
    
    using outgoing_item_type = item_type;
    static constexpr size_t BUFFER_SIZE = 1;
    static constexpr size_t MIN_BUFFER_SIZE = 1;
    
    template<typename context_type>
    class handler : public vds::sync_dataflow_source<context_type, handler<context_type>>
    {
    public:
      handler(
        const context_type & context,
        const dataflow_arguments & args)
      : vds::sync_dataflow_source<context_type, handler<context_type>>(context),
        data_(args.data_), count_(args.count_), written_(0)
      {
      }
      
      ~handler()
      {
      }
      
      size_t sync_get_data(const vds::service_provider & sp)
      {
        size_t count = 0;
        while(0 < this->count_ && count < this->output_buffer_size()){
          this->output_buffer(count) = *this->data_++;
          this->count_--;
          ++count;
        }
          
        return count;
      }
      
    private:
      const item_type * data_;
      size_t count_;
      size_t written_;
    };
  private:
    const item_type * data_;
    size_t count_;
  };
  
  template<typename item_type>
  class dataflow_consumer
  {
  public:
    typedef std::function<async_task<size_t> (const service_provider & sp, const item_type * data, size_t count)> callback_func_type;
    
    dataflow_consumer(const callback_func_type & callback_func)
    : callback_func_(callback_func)
    {
    }

    using incoming_item_type = item_type;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public async_dataflow_target<context_type, handler<context_type>>
    {
    public:
      handler(
        const context_type & context,
        const dataflow_consumer & args)
      : async_dataflow_target<context_type, handler<context_type>>(context),
        callback_func_(args.callback_func_)
      {
      }
      
      ~handler()
      {
      }
      
      void async_push_data(const service_provider & sp)
      {
        try {
          this->callback_func_(sp, this->input_buffer(), this->input_buffer_size())
          .wait([this](const service_provider & sp, size_t readed){
              if(this->processed(sp, readed)){
                this->async_push_data(sp);
              }
            },
            [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
              this->error(sp, ex);
            },
            sp);
        }
        catch(const std::exception & ex){
          this->error(sp, std::make_shared<std::runtime_error>(ex.what()));
        }
        catch(...){
          this->error(sp, std::make_shared<std::runtime_error>("Unexpected error"));
        }
      }
      
    private:
      callback_func_type callback_func_;
    };
    
  private:
    callback_func_type callback_func_;
  };
}

#endif // __VDS_CORE_DATAFLOW_H_
