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

    class sync_method
    {
    public:
      sync_method(test_async_object & owner)
      : owner_(owner)
      {
      }
      
      template<typename context_type>
      class handler : public vds::dataflow_step<context_type, void(const std::string &)>
      {
      public:
        handler(
          const context_type & context,
          const sync_method & owner
        ): vds::dataflow_step<
            context_type,
            void(const std::string &)>(context),
          owner_(owner.owner_){
        }
        
        void operator () (
          const vds::service_provider & sp,
          int value)
        {
          ASSERT_EQ(value, 10);
          ASSERT_EQ(this->owner_.state_, 0);

          this->owner_.state_++;
          this->next(sp, "test");
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
        vds::service_provider & sp,
        test_async_object & owner)
      : sp_(sp), owner_(owner)
      {
      }
      
      template<typename context_type>
      class handler : public vds::dataflow_step<context_type, void(void)>
      {
        using base_class = vds::dataflow_step<context_type, void(void)>;
      public:
        handler(
          const context_type & context,
          const async_method & owner
        )
        : base_class(context),
          owner_(owner.owner_),
          sp_(owner.sp_),
          async_task_body_(owner.owner_, this->next)
        {
        }
        
        void operator () (
          const vds::service_provider & sp,
          const std::string & value
        )
        {
          ASSERT_EQ(this->owner_.state_, 1);
          vds::imt_service::async(this->sp_, this->async_task_body_);
        }
        
      private:
        test_async_object & owner_;
        vds::service_provider & sp_;
        
        class async_body
        {
        public:
          async_body(
            test_async_object & owner,
            typename base_class::next_step_t & next)
          : next_(next), owner_(owner)
          {
          }
          
          void operator()()
          {
            ASSERT_EQ(this->owner_.state_, 1);

            this->owner_.state_++;
            this->next_(*(vds::service_provider *)nullptr);
          }
        private:
          typename base_class::next_step_t & next_;
          test_async_object & owner_;
        };
        
        async_body async_task_body_;
      };
      
    private:
      vds::service_provider & sp_;
      test_async_object & owner_;
    };

    int state_;
};


#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

