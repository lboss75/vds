#ifndef __VDS_CORE_DATAFLOW_H_
#define __VDS_CORE_DATAFLOW_H_

#include <set>
#include "cancellation_token.h"
#include "shutdown_event.h"
#include "service_provider.h"

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
      final_data_(false),
      waiting_get_data_(true),
      waiting_push_data_(false),
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
      if (!this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }
      
      this->waiting_get_data_ = false;

      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;

      if (this->waiting_push_data_) {
        return false;
      }
      
      if (nullptr == this->input_buffer_) {
        this->waiting_push_data_ = true;
        if (!this->source_->start(sp, this->input_buffer_, this->input_buffer_size_)) {
          return false;
        }
        this->waiting_push_data_ = false;
        this->readed_ = 0;

        if (0 == this->input_buffer_size_) {
          written = 0;
          return true;
        }
      }

      if (this->common_data_->cancellation_token_.is_cancellation_requested()) {
        this->source_->continue_read(sp, 0, this->input_buffer_, this->input_buffer_size_);
        this->final_data_ = true;
        this->common_data_->step_finish(sp, context_type::INDEX);
        return false;
      }

      for (;;) {

        if (0 == this->input_buffer_size_ && 0 < this->readed_) {
          this->waiting_push_data_ = true;
          if (!this->source_->continue_read(sp, this->readed_, this->input_buffer_, this->input_buffer_size_)) {
            return false;
          }
          this->waiting_push_data_ = false;
          this->readed_ = 0;
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

        if (0 < written) {
          if (readed > this->input_buffer_size_) {
            throw new std::runtime_error("Invalid login");
          }
          this->input_buffer_ += readed;
          this->input_buffer_size_ -= readed;
          this->readed_ += readed;

          if (this->waiting_get_data_) {
            throw std::runtime_error("Logic error");
          }

          this->waiting_get_data_ = true;
          return true;
        }
        else {
          if (0 == readed) {
            if (0 != this->input_buffer_size_) {
              throw new std::runtime_error("Invalid login");
            }

            this->final_data_ = true;
            if (this->common_data_->step_finish(sp, context_type::INDEX)) {
              return false;
            }
            return true;
          }

          this->waiting_push_data_ = true;
          this->readed_ += readed;
          if (!this->source_->continue_read(sp, this->readed_, this->input_buffer_, this->input_buffer_size_)) {
            return false;
          }

          this->waiting_push_data_ = false;
          this->readed_ = 0;
        }
      }
    }

    bool push_data(
      const service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {

      if (!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }
      this->waiting_push_data_ = false;

      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;
      this->readed_ = 0;

      if (this->waiting_get_data_) {
        return false;
      }

      size_t written;
      static_cast<implementation_type *>(this)->sync_process_data(sp, readed, written);


      if (0 < written) {
        if (readed > this->input_buffer_size_) {
          throw new std::runtime_error("Invalid login");
        }
      }
      else {
        if (0 == readed) {
          if (0 != this->input_buffer_size_) {
            throw new std::runtime_error("Invalid login");
          }

          this->final_data_ = true;
          if (this->common_data_->step_finish(sp, context_type::INDEX)) {
            return false;
          }
        }
      }

      if (0 < written || 0 == readed) {
        this->waiting_get_data_ = true;
        if (!this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
          return false;
        }

        this->waiting_get_data_ = false;
      }

      this->waiting_push_data_ = true;
      return !this->final_data_;
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

    bool final_data_;
    bool waiting_get_data_;
    bool waiting_push_data_;

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
      waiting_get_data_(true),
      waiting_push_data_(false),
      process_data_called_(false),
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
      if (!this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }

      this->waiting_get_data_ = false;

      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;

      if (this->waiting_push_data_ || this->process_data_called_) {
        return false;
      }

      if (nullptr == this->input_buffer_) {
        this->waiting_push_data_ = true;
        this->readed_ = 0;
        if (!this->source_->start(sp, this->input_buffer_, this->input_buffer_size_)) {
          return false;
        }
        this->waiting_push_data_ = false;
      }

      if (0 == this->input_buffer_size_) {
        readed = 0;
        return true;
      }

      this->process_data_called_ = true;
      static_cast<implementation_type *>(this)->async_process_data(sp);
      return false;
    }

    bool push_data(
      const service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & written)
    {
      if (!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }
      this->waiting_push_data_ = false;

      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;
      this->readed_ = 0;

      if (this->waiting_get_data_ || this->process_data_called_) {
        return false;
      }

      this->process_data_called_ = true;
      static_cast<implementation_type *>(this)->async_process_data(sp);

      return false;
    }
    
    bool processed(const service_provider & sp, size_t readed, size_t written)
    {
      if (this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }
      if (this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }
      
      if (!this->process_data_called_) {
        throw std::runtime_error("Logic error");
      }
      this->process_data_called_ = false;
      
      if (0 < readed) {
        if (0 == written) {
          this->waiting_push_data_ = true;
          if (this->source_->continue_read(sp, this->readed_ + readed, this->input_buffer_, this->input_buffer_size_)) {
            this->waiting_push_data_ = false;
            this->readed_ = 0;
          }
        }
        else {
          if (this->input_buffer_size_ < readed) {
            throw std::runtime_error("Login error");
          }

          this->input_buffer_ += readed;
          this->input_buffer_size_ -= readed;
          this->readed_ += readed;

          this->waiting_get_data_ = true;
          if (this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
            this->waiting_get_data_ = false;

            if (0 == this->input_buffer_size_) {
              if (this->waiting_push_data_) {
                throw std::runtime_error("Logic error");
              }

              this->waiting_push_data_ = true;
              if (this->source_->continue_read(sp, this->readed_, this->input_buffer_, this->input_buffer_size_)) {
                this->waiting_push_data_ = false;
                this->readed_ = 0;
              }
            }
          }
        }
      }
      else {
        if (0 == written) {
          if (0 != this->input_buffer_size_) {
            throw std::runtime_error("Logic error");
          }

          if (this->common_data_->step_finish(sp, context_type::INDEX)) {
            return false;
          }
          this->waiting_push_data_ = true;
        }

        this->waiting_get_data_ = true;
        if (this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
          this->waiting_get_data_ = false;
        }
        else {
          return false;
        }
      }
      
      if(!this->waiting_get_data_ && !this->waiting_push_data_){
        this->process_data_called_ = true;
        return true;
      }
      else {
        return false;
      }
    }

    bool cancel(const service_provider & sp)
    {
      return this->common_data_->step_finish(sp, context_type::INDEX);
    }

  private:
    incoming_queue_type * source_;
    outgoing_queue_type * target_;
    typename context_type::common_data_type * common_data_;

    bool waiting_get_data_;
    bool waiting_push_data_;
    bool process_data_called_;

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
      waiting_push_data_(false),
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
      if (!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }

      this->waiting_push_data_ = false;
      this->input_buffer_ = values;
      this->input_buffer_size_ = count;

      written = static_cast<implementation_type *>(this)->sync_push_data(sp);
      if (0 == written && 0 == count) {
        this->common_data_->step_finish(sp, context_type::INDEX);
        return false;
      }
      this->waiting_push_data_ = true;
      return true;
    }

    void start(const service_provider & sp)
    {
      if (this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }

      if (nullptr == this->input_buffer_) {
        this->waiting_push_data_ = true;
        if (!this->source_->start(sp, this->input_buffer_, this->input_buffer_size_)) {
          return;
        }
        
        this->waiting_push_data_ = false;
      }

      while(!this->common_data_->cancellation_token_.is_cancellation_requested()) {
        auto readed = static_cast<implementation_type *>(this)->sync_push_data(sp);

        if (0 == this->input_buffer_size_ && 0 == readed) {
          this->common_data_->step_finish(sp, context_type::INDEX);
          break;
        }

        this->waiting_push_data_ = true;
        if (this->source_->continue_read(sp, readed, this->input_buffer_, this->input_buffer_size_)) {
          this->waiting_push_data_ = false;
        }
        else {
          break;
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
    bool waiting_push_data_;

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
      waiting_push_data_(false),
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
      if (!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }

      this->waiting_push_data_ = false;
      this->input_buffer_ = values;
      this->input_buffer_size_ = count;

      static_cast<implementation_type *>(this)->async_push_data(sp);
      return false;
    }

    void start(const service_provider & sp)
    {
      if (this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }

      if (nullptr == this->input_buffer_) {
        this->waiting_push_data_ = true;
        if (!this->source_->start(sp, this->input_buffer_, this->input_buffer_size_)) {
          return;
        }
        
        this->waiting_push_data_ = false;
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
    bool waiting_push_data_;
    
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
      
      this->waiting_push_data_ = true;
      if(!this->source_->continue_read(sp, written, this->input_buffer_, this->input_buffer_size_)){
        return false;
      }
      
      this->waiting_push_data_ = false;
      return true;
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
    _dataflow(functor_types... functors)
    : builder_(std::forward<functor_types>(functors)...)
    {
    }
    
    void operator()(
      const std::function<void(const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp)
    {
      auto p = std::make_shared<starter>(done, on_error, this->builder_);
      p->start(p, sp);
    }        
    
  private:
    tuple_type builder_;

    class final_step_type;
    class starter;
    
    template<std::size_t index>
    using _functor_type_t = typename std::remove_reference<typename std::tuple_element<index, tuple_type>::type>::type;
    
    class common_data
    {
    public:
      common_data()
      : cancellation_token_(cancellation_source_.token())
      {
      }
      
      cancellation_token_source cancellation_source_;
      cancellation_token cancellation_token_;

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
        back_(0),
        data_final_(false),
        data_queried_(false),
        data_in_process_(false)
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
          if (0 == count && this->data_final_) {
            return false;
          }
          if(!this->data_queried_) {
            throw std::runtime_error("Logic error");
          }
          this->data_queried_ = false;

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

          if (this->front_ == this->back_ && !this->data_queried_ && !this->data_in_process_) {
            this->front_ = 0;
            this->back_ = this->second_;
            this->second_ = 0;
          }

          if (!this->data_in_process_) {
            incoming_item_type * read_buffer;
            size_t read_len;
            if(this->get_read_buffer(read_buffer, read_len)){
              this->data_in_process_ = true;
              size_t readed;
              if (!this->target_->push_data(sp, read_buffer, read_len, readed)) {
                return false;
              }
              this->data_in_process_ = false;
                
              this->front_ += readed;
                
              if (this->front_ == this->back_) {
                this->front_ = 0;
                this->back_ = this->second_;
                this->second_ = 0;
              }
            }
            else if (0 == count) {
              this->data_final_ = true;

              size_t readed;
              return this->target_->push_data(sp, nullptr, 0, readed);
            }
          }
          
          if (!this->get_write_buffer(buffer, buffer_len)) {
            return false;
          }

          this->data_queried_ = true;
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
      
      //from target
      bool start(
        const service_provider & sp,
        outgoing_item_type *& buffer,
        size_t & readed)
      {
       
        if(this->data_in_process_ || this->data_queried_) {
          throw std::runtime_error("Logic error");
        }

        size_t buffer_len;
        if(!this->get_write_buffer(buffer, buffer_len)){
          throw std::runtime_error("Logic error");
        }
        
        this->data_queried_ = true;
        if(this->source_->get_data(sp, buffer, buffer_len, readed)){
          this->back_ += readed;
          this->data_queried_ = false;
          this->data_in_process_ = true;
          return true;
        }
        return false;
      }
      
      bool continue_read(
        const service_provider & sp,
        size_t readed,
        outgoing_item_type *& buffer,
        size_t & buffer_len)
      {
        if(!this->data_in_process_) {
          if (this->data_final_ && this->common_data_->cancellation_token_.is_cancellation_requested()) {
            return false;
          }
          throw std::runtime_error("Logic error");
        }

        if (0 < readed) {
          this->front_ += readed;

          if (this->front_ < this->back_) {
            this->data_in_process_ = true;

            buffer = this->buffer_ + this->front_;
            buffer_len = this->back_ - this->front_;
            return true;
          }

          this->data_in_process_ = false;

          if (this->data_queried_) {
            return false;
          }

          if (this->front_ == this->back_ && !this->data_queried_ && !this->data_in_process_) {
            this->front_ = 0;
            this->back_ = this->second_;
            this->second_ = 0;
          }

          if (!this->get_write_buffer(buffer, buffer_len)) {
            return false;
          }
        }

        this->data_queried_ = true;
        if(this->source_->get_data(sp, buffer, buffer_len, readed)){
          this->data_queried_ = false;

          if (0 == readed) {
            this->data_final_ = true;
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
            throw std::runtime_error("Logic error");
          }

          if (!this->get_read_buffer(buffer, buffer_len)) {
            throw std::runtime_error("Logic error");
          }

          this->data_in_process_ = true;
          return true;
        }

        return false;
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

      bool data_final_;
      bool data_queried_;
      bool data_in_process_;

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
        tuple_type & args)
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
        tuple_type & args)
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
        tuple_type & args)
      : base_class(this, &queue_, args),
        done_(done), on_error_(on_error),
        step_(final_context(this, &queue_, this), std::get<std::tuple_size<tuple_type>::value - 1>(args)),
        queue_(this, base_class::step(), &step_)
      {
      }
      

      void start(const std::shared_ptr<starter> pthis, const service_provider & sp)
      {
        this->handler_token_ = sp.get_shutdown_event().then_shuting_down([this]() {
          this->cancellation_source_.cancel();
        });
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
          this->cancellation_source_.cancel();
        }

        this->done_steps_.emplace(index);
        return this->try_finish(sp);
      }


    private:
      std::function<void(const service_provider & sp)> done_;
      error_handler on_error_;
      final_step_type step_;
      queue_stream<std::tuple_size<tuple_type>::value - 2> queue_;
      cancellation_subscriber handler_token_;
      std::shared_ptr<starter> pthis_;

      std::set<size_t> done_steps_;
      std::shared_ptr<std::exception> error_;

      bool try_finish(const service_provider & sp)
      {
        if (std::tuple_size<tuple_type>::value > this->done_steps_.size()) {
          return false;
        }

        this->handler_token_.destroy();

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
  inline
  _dataflow<functor_types...> dataflow(functor_types&&... functors)
  {
    return _dataflow<functor_types...>(std::forward<functor_types>(functors)...);
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
}

#endif // __VDS_CORE_DATAFLOW_H_
