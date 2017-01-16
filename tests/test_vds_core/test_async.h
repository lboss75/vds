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
      
      template<
        typename done_method_type,
        typename error_method_type
      >
      class handler
      {
      public:
        handler(
          const done_method_type & done,
          const error_method_type & on_error,
          const sync_method & owner
        ): done_(done), on_error_(on_error),
        owner_(owner.owner_){
        }
        
        void operator () (
          int value
        ) const
        {
          ASSERT_EQ(value, 10);
          ASSERT_EQ(this->owner_.state_, 0);

          this->owner_.state_++;
          this->done_("test");
        }
        
      private:
        const done_method_type & done_;
        const error_method_type & on_error_;
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
      
      template<
        typename done_method_type,
        typename error_method_type
      >
      class handler
      {
      public:
        handler(
          done_method_type & done,
          error_method_type & on_error,
          const async_method & owner
        )
        : owner_(owner.owner_), sp_(owner.sp_),
        on_error_(on_error),
        async_task_body_(owner.owner_, done),
        async_task_(async_task_body_, on_error)
        {
        }
        
        void operator () (
          const std::string & value
        ) const
        {
          ASSERT_EQ(this->owner_.state_, 1);
          
          this->async_task_.schedule(this->sp_);
        }
        
      private:
        error_method_type & on_error_;
        test_async_object & owner_;
        vds::service_provider & sp_;
        
        class async_body
        {
        public:
          async_body(
            test_async_object & owner,
            const done_method_type & done)
          : owner_(owner), done_(done)
          {
          }
          
          void operator()() const
          {
            ASSERT_EQ(this->owner_.state_, 1);

            this->owner_.state_++;
            this->done_();
          }
        private:
          const done_method_type & done_;
          test_async_object & owner_;
        };
        
        async_body async_task_body_;
        vds::async_task<async_body, error_method_type> async_task_;
      };
      
    private:
      vds::service_provider & sp_;
      test_async_object & owner_;
    };

    int state_;
};


#endif // !__TEST_VDS_CORE_TEST_ASYNC_H_

