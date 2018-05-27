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
  class state_machine : public std::enable_shared_from_this<state_machine<state_enum_type>>
  {
  public:
    state_machine(state_enum_type start_state)
    : state_(start_state)
    {
    }
    
    async_task<> change_state(state_enum_type expected_state, state_enum_type new_state)
    {
      return [pthis = this->shared_from_this(), expected_state, new_state](const async_result<> & result){
        std::unique_lock<std::mutex> lock(pthis->state_mutex_);
        if(expected_state == pthis->state_){
          pthis->state_ = new_state;
          auto p = pthis->state_expectants_.find(new_state);
          if(pthis->state_expectants_.end() != p){
            auto callback = p->second;
            pthis->state_expectants_.erase(p);
            lock.unlock();

            callback.done();
          }

          result.done();
        }
        else {
          vds_assert(pthis->state_expectants_.end() == pthis->state_expectants_.find(expected_state));
          pthis->state_expectants_[expected_state] = result;
        }
      };
    }

    async_task<> wait(state_enum_type expected_state)
    {
      return [pthis = this->shared_from_this(), expected_state](const async_result<> & result){
        std::unique_lock<std::mutex> lock(pthis->state_mutex_);
        if(expected_state == pthis->state_){
          result.done();
        }
        else {
          vds_assert(pthis->state_expectants_.end() == pthis->state_expectants_.find(expected_state));
          pthis->state_expectants_[expected_state] = result;
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
