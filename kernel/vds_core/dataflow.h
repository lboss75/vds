#ifndef __VDS_CORE_DATAFLOW_H_
#define __VDS_CORE_DATAFLOW_H_

#include <set>
#include <mutex>
#include "cancellation_token.h"
#include "shutdown_event.h"
#include "mt_service.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  template <typename context_type, typename implementation_type>
  class sync_dataflow_target
  {
  public:
    using incoming_item_type = typename context_type::incoming_item_type;
    using incoming_queue_type = typename context_type::incoming_queue_type;
    sync_dataflow_target(const context_type & context)
    : source_(context.source_),
      terminator_(context.terminator_)
    {
    }
    
    sync_dataflow_target(const sync_dataflow_target &) = delete;
    sync_dataflow_target(sync_dataflow_target&&) = delete;
    sync_dataflow_target & operator = (const sync_dataflow_target &) = delete;
    sync_dataflow_target & operator = (sync_dataflow_target&&) = delete;
    
    bool push_data(
      const vds::service_provider & sp,
      const bool * values,
      size_t count,
      size_t & written)
    {
      written = static_cast<implementation_type *>(this)->sync_push_data(sp, values, count);
      return true;
    }

  private:
    incoming_queue_type * source_;
    typename context_type::terminator_type * terminator_;
  };

  template <typename context_type, typename implementation_type>
  class async_dataflow_target
  {
  public:
    using incoming_queue_type = typename context_type::incoming_queue_type;
    async_dataflow_target(const context_type & context)
      : source_(context.source_),
      terminator_(context.terminator_)
    {
    }

    async_dataflow_target(const async_dataflow_target &) = delete;
    async_dataflow_target(async_dataflow_target&&) = delete;
    async_dataflow_target & operator = (const async_dataflow_target &) = delete;
    async_dataflow_target & operator = (async_dataflow_target&&) = delete;

  private:
    incoming_queue_type * source_;
    typename context_type::terminator_type * terminator_;
  };

  template <typename context_type, typename implementation_type>
  class sync_dataflow_source
  {
  public:
    
    using outgoing_item_type = typename context_type::outgoing_item_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;
    
    sync_dataflow_source(const context_type & context)
    : target_(context.target_)
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
      readed = static_cast<implementation_type *>(this)->sync_get_data(sp, buffer, buffer_size);
      return true;
    }

    void start(const service_provider & sp)
    {
      this->target_->start(sp);
    }

  private:
    outgoing_queue_type * target_;
  };

  template <typename context_type, typename implementation_type>
  class async_dataflow_source
  {
  public:
    using outgoing_item_type = typename context_type::outgoing_item_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;

    async_dataflow_source(const context_type & context)
      : target_(context.target_)
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
      static_cast<implementation_type *>(this)->async_get_data(sp, buffer, buffer_size);
      return false;
    }

    void start(const service_provider & sp)
    {
      this->target_->start(sp);
    }

  private:
    outgoing_queue_type * target_;
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
      cancel_token_(context.cancel_token_),
      waiting_get_data_(true),
      waiting_push_data_(true)
    {
    }

    bool get_data(
      const vds::service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & written)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);
      if (!this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }
      this->waiting_get_data_ = false;

      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;

      if (this->waiting_push_data_) {
        return false;
      }

      size_t readed;
      static_cast<implementation_type *>(this)->sync_process_data(sp, readed, written);

      this->waiting_push_data_ = true;
      if (this->source_->continue_read(sp, readed, this->input_buffer_, this->input_buffer_size_)) {
        this->waiting_push_data_ = false;
      }

      return true;
    }

    bool push_data(
      const vds::service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);

      if (!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }
      this->waiting_push_data_ = false;

      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;

      if (this->waiting_get_data_) {
        return false;
      }

      size_t written;
      static_cast<implementation_type *>(this)->sync_process_data(sp, readed, written);

      this->waiting_get_data_ = true;

      if (this->target_->push_data(sp, written, this->output_buffer_, this->output_buffer_size_)) {
        this->waiting_get_data_ = false;
      }

      return true;
    }

    void start(const service_provider & sp)
    {
      this->target_->start(sp);
    }

  private:
    incoming_queue_type * source_;
    outgoing_queue_type * target_;
    cancellation_token & cancel_token_;

  protected:
    std::recursive_mutex data_mutex_;

    bool waiting_get_data_;
    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;

    bool waiting_push_data_;
    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;
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
      cancel_token_(context.cancel_token_),
      waiting_get_data_(true),
      waiting_push_data_(true),
      process_data_called_(false)
    {
    }
    
    bool get_data(
      const vds::service_provider & sp,
      outgoing_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);
      if (!this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }

      this->waiting_get_data_ = false;

      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;
      
      if (this->waiting_push_data_ || this->process_data_called_) {
        return false;
      }

      this->process_data_called_ = true;
      static_cast<implementation_type *>(this)->async_process_data(sp);

      return false;
    }
    
    bool push_data(
      const vds::service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & written)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);
      
      if(!this->waiting_push_data_) {
        throw std::runtime_error("Logic error");
      }
      this->waiting_push_data_ = false;

      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;
      
      if (this->waiting_get_data_ || this->process_data_called_) {
        return false;
      }

      this->process_data_called_ = true;
      static_cast<implementation_type *>(this)->async_process_data(sp);

      return false;
    }

    void start(const service_provider & sp)
    {
      this->target_->start(sp);
    }

    //bool process_data(const vds::service_provider & sp)
  private:
    incoming_queue_type * source_;
    outgoing_queue_type * target_;
    cancellation_token & cancel_token_;

    std::recursive_mutex data_mutex_;
    
    bool waiting_get_data_;
    incoming_item_type * input_buffer_;
    size_t input_buffer_size_;
    
    bool waiting_push_data_;
    outgoing_item_type * output_buffer_;
    size_t output_buffer_size_;
    
    bool process_data_called_;

  protected:
    bool processed(
      const vds::service_provider & sp,
      size_t input_processed,
      size_t output_processed)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);

      if(input_processed > this->input_buffer_size_ || output_processed > this->output_buffer_size_) {
        throw std::runtime_error("Logic error");
      }

      if (this->waiting_push_data_ || this->waiting_get_data_) {
        throw std::runtime_error("Logic error");
      }

      this->waiting_push_data_ = true;
      if(this->source_->continue_read(sp, input_processed, this->input_buffer_, this->input_buffer_size_)){
        this->waiting_push_data_ = false;
      }
      
      this->waiting_get_data_ = true;
      if(this->target_->push_data(sp, output_processed, this->output_buffer_, this->output_buffer_size_)){
        this->waiting_get_data_ = false;
      }
      
      return !this->waiting_get_data_ && !this->waiting_push_data_;
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

      virtual void step_finish(const service_provider & sp, size_t index) = 0;
      virtual void step_error(const service_provider & sp, size_t index, std::exception_ptr error) = 0;
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
      using incoming_item_type = typename _functor_type_t<index>::incoming_item_type;
      using outgoing_item_type = typename _functor_type_t<index>::outgoing_item_type;
      using incoming_queue_type = queue_stream<index - 1>;
      using outgoing_queue_type = queue_stream<index>;
      
      context(
        common_data * data,
        incoming_queue_type * source,
        outgoing_queue_type * target)
      : source_(source), target_(target),
        cancel_token_(data->cancellation_token_)
      {
      }
      
      incoming_queue_type * source_;
      outgoing_queue_type * target_;
      cancellation_token & cancel_token_;
    };
    
    template<
      typename enabled,
      bool dummy>
    class context<0, enabled, dummy>
    {
    public:
      using outgoing_item_type = typename _functor_type_t<0>::outgoing_item_type;
      using outgoing_queue_type = queue_stream<0>;
      
      context(common_data * data, outgoing_queue_type * target)
      : target_(target),
        cancel_token_(data->cancellation_token_)
      {
      }
      
      outgoing_queue_type * target_;
      cancellation_token & cancel_token_;
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
          std::lock_guard<std::mutex> lock(this->buffer_mutex_);
          if(!this->data_queried_) {
            throw std::runtime_error("Logic error");
          }
          this->data_queried_ = false;
          std::cout << "queue.push_data[" << index << "]: data_queried_=1->0,data_in_process_=" << data_in_process_ << "\n";
          if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
            this->back_ += count;
          }
          else {
            if (this->front_ == this->back_ && !this->data_queried_ && !this->data_in_process_) {
              this->front_ = 0;
              this->back_ = this->second_;
              this->second_ = 0;
            }

            if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
              this->second_ += count;
            }
            else {
              throw std::runtime_error("Invalid logic");
            }
          }

          if (!this->data_in_process_) {
            this->push_data(sp);
          }
          
          if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
            this->data_queried_ = true;
            std::cout << "queue.push_data[" << index << "]: data_queried_=0->1,data_in_process_=" << data_in_process_ << "\n";
            buffer = this->buffer_ + this->back_;
            buffer_len = BUFFER_SIZE - this->back_;
            return true;
          }

          if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
            this->data_queried_ = true;
            std::cout << "queue.push_data[" << index << "]: data_queried_=0->1,data_in_process_=" << data_in_process_ << "\n";
            buffer = this->buffer_ + this->second_;
            buffer_len = this->front_ - this->second_;
            return true;
          }

          
          return false;
        }
        catch(...){
          this->common_data_->step_error(sp, index, std::current_exception());
          return false;
        }
      }
      
      void final_data(const service_provider & sp)
      {
        std::lock_guard<std::mutex> lock(this->buffer_mutex_);
        this->data_final_ = true;
      }
      
      //from target
      bool continue_read(
        const service_provider & sp,
        size_t readed,
        outgoing_item_type *& buffer,
        size_t & buffer_len)
      {
        std::lock_guard<std::mutex> lock(this->buffer_mutex_);
        if(!this->data_in_process_) {
          throw std::runtime_error("Logic error");
        }
        this->front_ += readed;
        if (this->front_ < this->back_) {
          this->data_in_process_ = true;
          std::cout << "queue.continue_read[" << index << "]: data_queried_=" << data_queried_ << ",data_in_process_=0->1\n";
          buffer = this->buffer_ + this->front_;
          buffer_len = this->back_ - this->front_;
          return true;
        }
        this->data_in_process_ = false;
        std::cout << "queue.continue_read[" << index << "]: data_queried_=" << data_queried_ << ",data_in_process_=1->0\n";

        if (!this->data_queried_) {
          this->query_data(sp);
        }

        return false;
      }
      
      void start(
        const service_provider & sp)
      {
        this->query_data(sp);
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
      
      std::mutex buffer_mutex_;
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
      
      bool push_data(const service_provider & sp)
      {
        if(this->data_in_process_) {
          throw std::runtime_error("Logic error");
        }

        if (this->front_ < this->back_) {
          this->data_in_process_ = true;
          std::cout << "queue.continue_read[" << index << "]: data_queried_=" << data_queried_ << ",data_in_process_=0->1\n";
          auto p = this->buffer_ + this->front_;
          auto l = this->back_ - this->front_;
          
          imt_service::async(sp, [this, sp, p, l](){
            incoming_item_type * buffer = p;
            size_t len = l;
            size_t readed;
            while(this->target_->push_data(sp, buffer, len, readed)
              && this->continue_read(sp, readed, buffer, len)){
            }
          });

          return true;
        }

        return false;
      }
      
      bool query_data(const service_provider & sp)
      {
        if(this->data_queried_) {
          throw std::runtime_error("Logic error");
        }

        if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
          this->data_queried_ = true;
          std::cout << "queue.query_data[" << index << "]: data_queried_=0->1,data_in_process_=" << data_in_process_ << "\n";
          auto p = this->buffer_ + this->back_;
          auto l = BUFFER_SIZE - this->back_;
          imt_service::async(sp, [this, sp, p, l](){
            size_t readed;
            if(this->source_->get_data(sp, p, l, readed)){
              std::lock_guard<std::mutex> lock(this->buffer_mutex_);
              this->back_ += readed;
              this->query_data(sp);
            }
          });
          return true;
        }

        if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
          this->data_queried_ = true;
          std::cout << "queue.query_data[" << index << "]: data_queried_=0->1,data_in_process_=" << data_in_process_ << "\n";
          auto p = this->buffer_ + this->second_;
          auto l = this->front_ - this->second_;
          
          imt_service::async(sp, [this, sp, p, l](){
            if(this->source_->get_data(sp, p, l, readed)){
              std::lock_guard<std::mutex> lock(this->buffer_mutex_);
              this->second_ += readed;
              this->query_data(sp, readed);
            }
          });
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
      
      void start(const service_provider & sp)
      {
        this->step_.start(sp);
        base_class::start(sp);
      }
      
      step_type<index> * step(){
        return &this->step_;
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
      
      void start(const service_provider & sp)
      {
        this->step_.start(sp);
      }
      
      step_type<0> * step(){
        return &this->step_;
      }
      
    private:
      step_type<0> step_;
    };
    
    class final_context
    {
    public:
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
        source_(ctx.source_)
      {
      }
      
    private:
      incoming_queue_type * source_;
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
        this->pthis_ = pthis;
        base_class::start(sp);
      }
      
      void final_data(const service_provider & sp)
      {
        cancellation_source_.cancel();
      }

      void step_finish(const service_provider & sp, size_t index) override
      {
        this->final_mutex_.lock();
        this->done_steps_.emplace(index);
        this->try_finish(sp);
      }

      void step_error(const service_provider & sp, size_t index, std::exception_ptr error) override
      {
        this->final_mutex_.lock();
        if (!this->error_) {
          this->error_ = error;
          cancellation_source_.cancel();
        }
        this->done_steps_.emplace(index);
        this->try_finish(sp);
      }


    private:
      std::function<void(const service_provider & sp)> done_;
      error_handler on_error_;
      final_step_type step_;
      queue_stream<std::tuple_size<tuple_type>::value - 2> queue_;
      std::shared_ptr<starter> pthis_;

      std::mutex final_mutex_;
      std::set<size_t> done_steps_;
      std::exception_ptr error_;

      void try_finish(const service_provider & sp)
      {
        if (std::tuple_size<tuple_type>::value > this->done_steps_.size()) {
          this->final_mutex_.unlock();
          return;
        }
        this->final_mutex_.unlock();

        if (!this->error_) {
          this->done_(sp);
        }
        else {
          this->on_error_(sp, this->error_);
        }

        this->pthis_.reset();
      }
    };
  };
  
  template <typename... functor_types>
  inline
  _dataflow<functor_types...> dataflow(functor_types&&... functors)
  {
    return _dataflow<functor_types...>(std::forward<functor_types>(functors)...);
  }
  
}

#endif // __VDS_CORE_DATAFLOW_H_
