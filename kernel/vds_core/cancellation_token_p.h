#ifndef __VDS_CORE_CANCELLATION_TOKEN_P_H_
#define __VDS_CORE_CANCELLATION_TOKEN_P_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <mutex>
#include <map>

namespace vds {
  class _cancellation_token
  : public std::enable_shared_from_this<_cancellation_token>
  {
  public:
    _cancellation_token()
    : is_cancellation_requested_(false), last_callback_(0)
    {
    }
    
    bool is_cancellation_requested() const
    {
      return this->is_cancellation_requested_;
    }
    
    cancellation_subscriber then_cancellation_requested(
      const std::shared_ptr<_cancellation_token> & pthis,
      const std::function<void(void)> & callback)
    {
      std::lock_guard<std::mutex> lock(this->callbacks_mutex_);
      if(this->is_cancellation_requested_){
        callback();
        return cancellation_subscriber();
      }
      else {
        auto index = ++(this->last_callback_);
        this->callbacks_[index] = callback;
        return cancellation_subscriber(index, pthis);
      }
    }
    
    void remove_callback(int index)
    {
      std::lock_guard<std::mutex> lock(this->callbacks_mutex_);
      this->callbacks_.erase(index);
    }
    
    void cancel()
    {
      std::lock_guard<std::mutex> lock(this->callbacks_mutex_);
      this->is_cancellation_requested_ = true;
      for(auto & callback : this->callbacks_) {
        callback.second();
      }
      
      this->callbacks_.clear();
    }
    
  private:
    bool is_cancellation_requested_;
    std::mutex callbacks_mutex_;
    int last_callback_;
    std::map<int, std::function<void(void)>> callbacks_;
  };
}

#endif // __VDS_CORE_CANCELLATION_TOKEN_P_H_
