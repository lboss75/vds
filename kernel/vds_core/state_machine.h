#ifndef __VDS_CORE_STATE_MACHINE_H_
#define __VDS_CORE_STATE_MACHINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <mutex>
#include <map>
#include <condition_variable>

#include "types.h"
#include "vds_debug.h"

namespace vds {
  
  template <typename state_enum_type>
  class state_machine
  {
  public:
    state_machine(state_enum_type start_state)
    : state_(start_state)
    {
    }
    
    async_task<> change_state(state_enum_type expected_state, state_enum_type new_state)
    {
      return [this, expected_state, new_state](const async_result<> & result){
        std::unique_lock<std::mutex> lock(this->state_mutex_);
        if(expected_state == this->state_){
          this->state_ = new_state;
          auto p = this->state_expectants_.find(new_state);
          if(this->state_expectants_.end() != p){
            auto callback = p->second;
            this->state_expectants_.erase(p);

            callback.done();
          }

          result.done();
        }
        else {
          vds_assert(this->state_expectants_.end() == this->state_expectants_.find(expected_state));
          this->state_expectants_[expected_state] = result;
        }
      };
    }

    async_task<> wait(state_enum_type expected_state)
    {
      return [this, expected_state](const async_result<> & result){
        std::unique_lock<std::mutex> lock(this->state_mutex_);
        if(expected_state == this->state_){
          result.done();
        }
        else {
          vds_assert(this->state_expectants_.end() == this->state_expectants_.find(expected_state));
          this->state_expectants_[expected_state] = result;
        }
      };
    }


  private:
    state_enum_type state_;

    mutable std::mutex state_mutex_;
    std::map<state_enum_type, async_result<>> state_expectants_;
  };
  
};

#endif//__VDS_CORE_STATE_MACHINE_H_
