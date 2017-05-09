#ifndef __TEST_VDS_CORE_TEST_ASYNC_H_
#define __TEST_VDS_CORE_TEST_ASYNC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

class test_async_object
{
public:
    test_async_object();
    class source_method
    {
    public:
      source_method(test_async_object & owner)
      : owner_(owner)
      {
      }
      using outgoing_item_type = int;
      static constexpr size_t BUFFER_SIZE = 1024;
      static constexpr size_t MIN_BUFFER_SIZE = 1;
      
      template<typename context_type>
      class handler : public vds::dataflow_source<context_type>
      {
      public:
        handler(
          const context_type & context,
          const source_method & owner)
        : vds::dataflow_source<context_type>(context),
          owner_(owner.owner_), written_(0)
        {
        }
        
        ~handler()
        {
        }
        
        bool get_data(
          const vds::service_provider & sp,
          int * buffer,
          size_t buffer_size)
        {
          std::cout << "source_method::get_data(" << buffer_size << ")\n";
          for(;;){
            size_t count = 0;
            while(this->written_ < 2000 && count < buffer_size){
              *buffer++ = this->written_;
              ++count;
              this->written_++;
            }
            
            if(count > 0){
              std::cout << "source_method::get_data->push_data " << count << "\n";
              if(!this->target_->push_data(sp, count, buffer, buffer_size)){
                std::cout << "source_method::get_data->push_data returns false\n";
                return false;
              }
              std::cout << "source_method::get_data->push_data returns buffer " << buffer_size << "\n";
              
              continue;
            }
            
            if(10000 == this->written_){
              std::cout << "source_method::get_data->final_data\n";
              this->target_->final_data(sp);
            }
            
            std::cout << "source_method::get_data return false\n";
            return false;
          }
        }
        
      private:
        test_async_object & owner_;
        size_t written_;
      };
    private:
      test_async_object & owner_;
    };

    class sync_method
    {
    public:
      sync_method(test_async_object & owner)
      : owner_(owner)
      {
      }
      
      using incoming_item_type = int;
      using outgoing_item_type = std::string;
      static constexpr size_t BUFFER_SIZE = 532;
      static constexpr size_t MIN_BUFFER_SIZE = 1;
      
      template<typename context_type>
      class handler : public vds::dataflow_filter<context_type, handler<context_type>>
      {
      public:
        handler(
          const context_type & context,
          const sync_method & owner)
        : vds::dataflow_filter<context_type, handler<context_type>>(context),
          owner_(owner.owner_)
        {
        }
        
        bool process_data(const vds::service_provider & sp)
        {
          std::cout << "sync_method::process_data(input = " << this->input_buffer_size_ << ", output = " << this->output_buffer_size_ << ")\n";
          for(;;){
            auto n = (this->input_buffer_size_ < this->output_buffer_size_)
              ? this->input_buffer_size_
              : this->output_buffer_size_;
            
            for(size_t i = 0; i < n; ++i){
              this->output_buffer_[i] = std::to_string(this->input_buffer_[i]);
            }
            
            std::cout << "sync_method::process_data>processed(" << n << ")\n";
            if(!this->processed(sp, n, n)){
              std::cout << "sync_method::process_data>processed returns false\n";
              return false;
            }
            std::cout << "sync_method::process_data continue with (input = " << this->input_buffer_size_ << ", output = " << this->output_buffer_size_ << ")\n";
          }
        }
        
      private:
        test_async_object & owner_;
      };
      
    private:
      test_async_object & owner_;
    };
    
    class async_method
    {
    public:
      async_method(
        test_async_object & owner)
      : owner_(owner)
      {
      }
      
      using incoming_item_type = std::string;
      using outgoing_item_type = bool;
      static constexpr size_t BUFFER_SIZE = 712;
      static constexpr size_t MIN_BUFFER_SIZE = 1;
      
      template<typename context_type>
      class handler : public vds::dataflow_filter<context_type, handler<context_type>>
      {
        using base_class = vds::dataflow_filter<context_type, handler<context_type>>;
      public:
        handler(
          const context_type & context,
          const async_method & owner
        )
        : base_class(context),
          owner_(owner.owner_)
        {
        }
        
        bool process_data(const vds::service_provider & sp)
        {
          vds::imt_service::async(sp, [this, sp](){
            std::cout << "async_method::process_data(input = " << this->input_buffer_size_ << ", output = " << this->output_buffer_size_ << ")\n";
            for(;;){
              auto n = (this->input_buffer_size_ < this->output_buffer_size_)
                ? this->input_buffer_size_
                : this->output_buffer_size_;
              
              for(size_t i = 0; i < n; ++i){
                this->output_buffer_[i] = (this->input_buffer_[i] == std::to_string(i));
              }
              
              std::cout << "async_method::process_data>processed(" << n << ")\n";
              if(!this->processed(sp, n, n)){
                std::cout << "async_method::process_data>processed returns false\n";
                break;
              }
              std::cout << "async_method::process_data continue with (input = " << this->input_buffer_size_ << ", output = " << this->output_buffer_size_ << ")\n";
            }
          });
          
          return false;
        }
        
      private:
        test_async_object & owner_;
      };
      
    private:
      test_async_object & owner_;
    };
    
    class target_method
    {
    public:
      target_method(
        test_async_object & owner)
        : owner_(owner)
      {
      }
      
      using incoming_item_type = bool;
      static constexpr size_t BUFFER_SIZE = 1024;
      static constexpr size_t MIN_BUFFER_SIZE = 1;
      
      template<typename context_type>
      class handler : public vds::dataflow_target<context_type>
      {
        using base_class = vds::dataflow_target<context_type>;
      public:
        handler(
          const context_type & context,
          const target_method & owner
        )
        : base_class(context),
          owner_(owner.owner_)
        {
        }
        
        bool push_data(
          const vds::service_provider & sp,
          const bool * values,
          size_t count)
        {
          std::cout << "target_method::push_data(" << count << ")\n";
          for(size_t i = 0; i < count; ++i) {
            if(true != values[i]) {
              std::cout << "target_method::push_data test failed\n";
              throw std::runtime_error("test failed");
            }
          }
          
          return true;
        }
        
        void final_data()
        {
          std::cout << "target_method::final_data\n";
          
          if(!this->is_done_) {
            throw std::runtime_error("test failed");
          }
          
          this->terminator_->final_data();
        }
        
      private:
        test_async_object & owner_;
      };
    private:
      test_async_object & owner_;
    };
    int state_;
};


#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

