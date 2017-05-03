#ifndef __VDS_CORE_CANCELLATION_TOKEN_P_H_
#define __VDS_CORE_CANCELLATION_TOKEN_P_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _cancellation_token
  : public std::enable_shared_from_this<_cancellation_token>
  {
  public:
    _cancellation_token()
    : is_cancellation_requested_(false)
    {
    }
    
    bool is_cancellation_requested() const
    {
      return this->is_cancellation_requested_;
    }
    
    void then_cancellation_requested(
      const std::function<void(void)> & callback)
    {
      std::lock_guard<std::mutex> lock(this->callbacks_mutex_);
      if(this->is_cancellation_requested_){
        callback();
      }
      else {
        this->callbacks_.push_back(callback);
      }
    }
    
    void cancel()
    {
      std::lock_guard<std::mutex> lock(this->callbacks_mutex_);
      this->is_cancellation_requested_ = true;
      for(auto & callback : this->callbacks_) {
        callback();
      }
      
      this->callbacks_.clear();
    }
    
  private:
    bool is_cancellation_requested_;
    std::mutex callbacks_mutex_;
    std::list<std::function<void(void)>> callbacks_;
  };
}

#endif // __VDS_CORE_CANCELLATION_TOKEN_P_H_
