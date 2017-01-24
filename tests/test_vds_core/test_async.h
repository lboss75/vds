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
      class handler : public vds::sequence_step<context_type, void(const std::string &)>
      {
      public:
        handler(
          const context_type & context,
          const sync_method & owner
        ): base(context),
          owner_(owner.owner_){
        }
        
        void operator () (
          int value
        )
        {
          ASSERT_EQ(value, 10);
          ASSERT_EQ(this->owner_.state_, 0);

          this->owner_.state_++;
          this->next("test");
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
      : owner_(owner), sp_(sp)
      {
      }
      
      template<typename context_type>
      class handler : public vds::sequence_step<context_type, void(void)>
      {
      public:
        handler(
          const context_type & context,
          const async_method & owner
        )
        : base(context),
          owner_(owner.owner_), sp_(owner.sp_),
          async_task_body_(owner.owner_, next),
          async_task_(async_task_body_, error)
        {
        }
        
        void operator () (
          const std::string & value
        )
        {
          ASSERT_EQ(this->owner_.state_, 1);
          
          this->async_task_.schedule(this->sp_);
        }
        
      private:
        test_async_object & owner_;
        vds::service_provider & sp_;
        
        class async_body
        {
        public:
          async_body(
            test_async_object & owner,
            next_step_t & next)
          : owner_(owner), next_(next)
          {
          }
          
          void operator()()
          {
            ASSERT_EQ(this->owner_.state_, 1);

            this->owner_.state_++;
            this->next_();
          }
        private:
          next_step_t & next_;
          test_async_object & owner_;
        };
        
        async_body async_task_body_;
        vds::async_task<async_body, error_method_t> async_task_;
      };
      
    private:
      vds::service_provider & sp_;
      test_async_object & owner_;
    };

    int state_;
};


#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

