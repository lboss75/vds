#ifndef __VDS_CORE_PIPELINE_QUEUE_H_
#define __VDS_CORE_PIPELINE_QUEUE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "dataflow.h"
#include "mt_service.h"

namespace vds {

  template <typename message_type, typename target_type>
  class pipeline_queue
  {
  public:
    pipeline_queue(const service_provider & sp)
    : mt_service_(sp.get<imt_service>())
    {
    }
    
    void push(const message_type & message)
    {
      std::unique_lock<std::mutex> lock(this->message_mutex_);
      this->messages_.push_back(message);
      this->try_to_run();
    }
    
    void get(target_type & target)
    {
      std::unique_lock<std::mutex> lock(this->message_mutex_);
      this->callbacks_.push_back(std::unique_ptr<callback_handler>(new callback_handler(target)));
      this->try_to_run();
    }
    
  private:
    struct callback_handler
    {
      callback_handler(
        target_type & target)
      : target_(target)
      {
      }
      
      bool filter_messages(std::list<message_type> & messages)
      {
        return this->target_.filter_messages(
          messages,
          this->messages_);
      }
      
      void run()
      {
        this->target_.run(this->messages_);
      }
      
      target_type & target_;
      std::list<message_type> messages_;
    };
    
    std::mutex message_mutex_;
    std::list<message_type> messages_;
    std::list<std::unique_ptr<callback_handler>> callbacks_;
    imt_service mt_service_;
    
    bool try_to_run()
    {
      if(this->messages_.empty() || this->callbacks_.empty()){
        return false;
      }
      
      for(auto & c : this->callbacks_){
        if(c->filter_messages(this->messages_)){
          callback_handler * s = c.release();
          this->callbacks_.remove(c);

          this->mt_service_.async([s]() { std::unique_ptr<callback_handler>(s)->run(); });

          return true;
        }
      }
      
      return false;
    }
  };
}

#endif // __VDS_CORE_PIPELINE_QUEUE_H_
