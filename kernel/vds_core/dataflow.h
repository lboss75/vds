#ifndef __VDS_CORE_DATAFLOW_H_
#define __VDS_CORE_DATAFLOW_H_
#include "mt_service.h"

#include <assert.h>
#include "cancellation_token.h"
#include "shutdown_event.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  template <typename context_type>
  class dataflow_target
  {
  public:
    using incoming_queue_type = typename context_type::incoming_queue_type;
    dataflow_target(const context_type & context)
    : source_(context.source_),
      terminator_(context.terminator_)
    {
    }
    
    dataflow_target(const dataflow_target &) = delete;
    dataflow_target(dataflow_target&&) = delete;
    dataflow_target & operator = (const dataflow_target &) = delete;
    dataflow_target & operator = (dataflow_target&&) = delete;
    
    incoming_queue_type * source_;
    typename context_type::terminator_type * terminator_;
  };

  template <typename context_type>
  class dataflow_source
  {
  public:
    
    using outgoing_queue_type = typename context_type::outgoing_queue_type;
    
    dataflow_source(const context_type & context)
    : target_(context.target_)
    {
    }
    
    dataflow_source(const dataflow_source &) = delete;
    dataflow_source(dataflow_source&&) = delete;
    dataflow_source & operator = (const dataflow_source &) = delete;
    dataflow_source & operator = (dataflow_source&&) = delete;
    
    outgoing_queue_type * target_;
  };
  
  template <typename context_type, typename implementation_type>
  class dataflow_filter
  {
  public:
    
    using incoming_item_type = typename context_type::incoming_item_type;
    using outgoing_item_type = typename context_type::outgoing_item_type;
    
    using incoming_queue_type = typename context_type::incoming_queue_type;
    using outgoing_queue_type = typename context_type::outgoing_queue_type;
    
    dataflow_filter(const context_type & context)
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
      size_t buffer_size)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);
      assert(this->waiting_get_data_);
      this->waiting_get_data_ = false;
      
      this->output_buffer_ = buffer;
      this->output_buffer_size_ = buffer_size;
      
      return this->try_to_process(sp);
    }
    
    bool push_data(
      const vds::service_provider & sp,
      incoming_item_type * buffer,
      size_t buffer_size,
      size_t & readed)
    {
      std::lock_guard<std::recursive_mutex> lock(this->data_mutex_);
      
      assert(this->waiting_push_data_);
      this->waiting_push_data_ = false;
      
      this->input_buffer_ = buffer;
      this->input_buffer_size_ = buffer_size;
      
      return this->try_to_process(sp);
    }

    
    //bool process_data(const vds::service_provider & sp)
    
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
    
    bool process_data_called_;
    
    bool try_to_process(const vds::service_provider & sp)
    {
      if(this->waiting_get_data_
        || this->waiting_push_data_
        || this->process_data_called_){
        return false;
      }
      
      while(!this->cancel_token_.is_cancellation_requested()
        && !sp.get_shutdown_event().is_shuting_down()){
        this->process_data_called_ = true;
        if(!static_cast<implementation_type *>(this)->process_data(sp)){
          return false;
        }
        this->process_data_called_ = false;
      
        if(0 == this->input_buffer_size_){
          return true;
        }
        
        if(0 == this->output_buffer_size_){
        }      
      }
    }
    
    bool processed(
      const vds::service_provider & sp,
      size_t input_processed,
      size_t output_processed)
    {
      assert(input_processed <= this->input_buffer_size_);
      
      this->waiting_get_data_ = true;
      if(this->source_->continue_read(sp, input_processed, this->input_buffer_, this->input_buffer_size_)){
        this->waiting_get_data_ = false;
      }
      
      this->waiting_push_data_ = true;
      if(this->target_->push_data(sp, output_processed, this->output_buffer_, this->output_buffer_size_)){
        this->waiting_push_data_ = false;
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
      
      void on_error(const service_provider & sp, std::exception_ptr error)
      {
        this->cancellation_source_.cancel();
      }
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

          this->data_queried_ = false;
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
            buffer = this->buffer_ + this->back_;
            buffer_len = BUFFER_SIZE - this->back_;
            return true;
          }

          if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
            this->data_queried_ = true;
            buffer = this->buffer_ + this->second_;
            buffer_len = this->front_ - this->second_;
            return true;
          }

          
          return false;
        }
        catch(...){
          this->common_data_->on_error(sp, std::current_exception());
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
        assert(this->data_in_process_ == true);
        this->front_ += readed;
        if (this->front_ < this->back_) {
          this->data_in_process_ = true;
          buffer = this->buffer_ + this->front_;
          buffer_len = this->back_ - this->front_;
          return true;
        }
        this->data_in_process_ = false;
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
        assert(this->data_in_process_ == false);

        if (this->front_ < this->back_) {
          this->data_in_process_ = true;
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
        assert(this->data_queried_ == false);

        if (this->back_ + MIN_BUFFER_SIZE < BUFFER_SIZE) {
          this->data_queried_ = true;
          auto p = this->buffer_ + this->back_;
          auto l = BUFFER_SIZE - this->back_;
          imt_service::async(sp, [this, sp, p, l](){
            if(this->source_->get_data(sp, p, l)){
              this->query_data(sp);
            }
          });
          return true;
        }

        if (this->second_ + MIN_BUFFER_SIZE < this->front_) {
          this->data_queried_ = true;
          auto p = this->buffer_ + this->second_;
          auto l = this->front_ - this->second_;
          
          imt_service::async(sp, [this, sp, p, l](){
            if(this->source_->get_data(sp, p, l)){
              this->query_data(sp);
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
        this->step_.target_->start(sp);
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
        this->step_.target_->start(sp);
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
        this->done_(sp);
        this->pthis_.reset();
      }

    private:
      std::function<void(const service_provider & sp)> done_;
      error_handler on_error_;
      final_step_type step_;
      queue_stream<std::tuple_size<tuple_type>::value - 2> queue_;
      std::shared_ptr<starter> pthis_;
    };
    
    bool is_stopping_;
  };
  
  template <typename... functor_types>
  inline
  _dataflow<functor_types...> dataflow(functor_types&&... functors)
  {
    return _dataflow<functor_types...>(std::forward<functor_types>(functors)...);
  }
  
}

#endif // __VDS_CORE_DATAFLOW_H_
