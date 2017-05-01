#ifndef __VDS_CORE_PIPELINE_H_
#define __VDS_CORE_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "pipeline_queue.h"
#include "proxy_type_builder.h"
#include "func_utils.h"

namespace vds {
  class service_provider;

  template <typename... argument_types>
  class pipeline
  {
  public:
   
    void push(const service_provider & sp, argument_types... arguments)
    {
      this->queue_.push(sp, std::tuple<const service_provider &, argument_types...>(sp, arguments...));
    }
    
    template <typename target_type>
    void get(const service_provider & sp, target_type & target)
    {
      this->queue_.get(sp, this->proxy_type_builder_.get<target_type>(target));
    }
    
  private:
    using message_type = std::tuple<const service_provider &, argument_types...>;
    
    class imessage_sender
    {
    public:
      virtual ~imessage_sender() {}
      
      virtual bool filter_messages(
        const service_provider & sp,
        std::list<message_type> & source,
        std::list<message_type> & target) = 0;
        
      virtual void run(
        const service_provider & sp,
        std::list<message_type> & messages) = 0;
    };
    
    template <typename target_type>
    class message_sender : public imessage_sender
    {
    public:
      message_sender(target_type & target)
      : target_(target)
      {
      }
      
      bool filter_messages(
        const service_provider & sp,
        std::list<message_type> & source,
        std::list<message_type> & target) override
      {
        auto message = *source.begin();
        source.pop_front();
        target.push_back(message);
        return true;
      }
      
      void run(
        const service_provider & sp, 
        std::list<message_type> & messages) override
      {
        call_with(target_, *messages.begin());
      }
      
    private:
      target_type & target_;
    };
    
    proxy_type_builder<imessage_sender, message_sender> proxy_type_builder_;
    pipeline_queue<message_type, imessage_sender> queue_;

  };
}

#endif // ! __VDS_CORE_PIPELINE_H_

