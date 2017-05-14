#ifndef __VDS_CORE_PIPELINE_QUEUE_H_
#define __VDS_CORE_PIPELINE_QUEUE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include "dataflow.h"
#include "mt_service.h"

namespace vds {

  template <typename message_type, typename target_type>
  class pipeline_queue
  {
  public:
   
    void push(const service_provider & sp, const message_type & message)
    {
      std::unique_lock<std::mutex> lock(this->message_mutex_);
      this->messages_.push_back(message);
      this->try_to_run(sp);
    }
    
    bool get(const service_provider & sp, target_type & target)
    {
      std::unique_lock<std::mutex> lock(this->message_mutex_);
      this->callbacks_.push_back(std::unique_ptr<callback_handler>(new callback_handler(target)));
      return this->try_to_run(sp);
    }
    
  private:
    struct callback_handler
    {
      callback_handler(
        target_type & target)
      : target_(target)
      {
      }
      
      bool filter_messages(const service_provider & sp, std::list<message_type> & messages)
      {
        return this->target_.filter_messages(
          sp,
          messages,
          this->messages_);
      }
      
      void run(const service_provider & sp)
      {
        this->target_.run(sp, this->messages_);
      }
      
      target_type & target_;
      std::list<message_type> messages_;
    };
    
    std::mutex message_mutex_;
    std::list<message_type> messages_;
    std::list<std::unique_ptr<callback_handler>> callbacks_;
    
    bool try_to_run(const service_provider & sp)
    {
      if(this->messages_.empty() || this->callbacks_.empty()){
        return false;
      }
      
      for(auto & c : this->callbacks_){
        if(c->filter_messages(sp, this->messages_)){
          callback_handler * s = c.release();
          this->callbacks_.remove(c);

          imt_service::async(sp, [s, sp]() { std::unique_ptr<callback_handler>(s)->run(sp); });

          return true;
        }
      }
      
      return false;
    }
  };
}

#endif // __VDS_CORE_PIPELINE_QUEUE_H_
