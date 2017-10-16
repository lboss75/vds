#ifndef __VDS_CORE_STATE_MACHINE_H_
#define __VDS_CORE_STATE_MACHINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <mutex>
#include <condition_variable>

namespace vds {
  
  template <typename state_enum_type>
  class state_machine
  {
  public:
    state_machine(state_enum_type start_state, state_enum_type failed_state)
    : state_(start_state), failed_state_(failed_state)
    {
    }
    
    bool change_state(state_enum_type expected_state, state_enum_type new_state)
    {
      std::unique_lock<std::mutex> lock(this->state_);
      for(;;){
        if(expected_state == this->state_){
          this->state_ = new_state;
          this->state_cond_.notify_one();
          return true;
        }
        else if(this->failed_state_ == this->state_) {
          return false;
        }
        
        this->state_cond_.wait();
      }
    }
    
    bool fail()
    {
      std::unique_lock<std::mutex> lock(this->state_);
      if(this->failed_state_ != this->state_){
        this->state_ = this->failed_state_;
        this->state_cond_.notify_one();
        return true;
      }
      else {
        return false;
      }
    }
    
  private:
    state_enum_type state_;
    const state_enum_type failed_state_;
    std::mutex state_mutex_;
    std::condition_variable state_cond_;
  };
  
};

#endif//__VDS_CORE_STATE_MACHINE_H_
