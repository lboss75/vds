#ifndef __VDS_CORE_STATE_MACHINE_H_
#define __VDS_CORE_STATE_MACHINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <mutex>
#include <map>

#include "types.h"
#include "vds_debug.h"
#include "async_task.h"

namespace vds {
  
  template <typename state_enum_type>
  class state_machine : public std::enable_shared_from_this<state_machine<state_enum_type>>
  {
  public:
    state_machine(state_enum_type start_state)
    : state_(start_state)
    {
    }
    
    vds::async_task<expected<void>> change_state(state_enum_type expected_state, state_enum_type new_state)
    {
        std::unique_lock<std::mutex> lock(this->state_mutex_);
        if(expected_state == this->state_){
          this->state_ = new_state;
          lock.unlock();

          for (;;) {
            lock.lock();
            auto p = this->state_expectants_.find(this->state_);
            if (this->state_expectants_.end() != p) {
              this->state_ = std::get<0>(p->second);
              auto & callback = std::get<1>(p->second);
              this->state_expectants_.erase(p);
              lock.unlock();

              callback.set_value(expected<void>());
            }
            else {
              break;
            }
          }
          co_return expected<void>();
        }
        else {
          vds_assert(this->state_expectants_.end() == this->state_expectants_.find(expected_state));
          vds::async_result<vds::expected<void>> result;
          auto f = result.get_future();
          this->state_expectants_[expected_state] = std::make_tuple(new_state, std::move(result));
          co_return std::move(co_await std::move(f));
        }

      co_return expected<void>();
    }

    vds::async_task<vds::expected<void>> wait(state_enum_type expected_state)
    {
      std::unique_lock<std::mutex> lock(this->state_mutex_);
      if(expected_state != this->state_){
        vds_assert(this->state_expectants_.end() == this->state_expectants_.find(expected_state));
        vds::async_result<vds::expected<void>> result;
        auto ret = result.get_future();
        this->state_expectants_[expected_state] = std::make_tuple(expected_state, std::move(result));
        co_return std::move(co_await std::move(ret));
      }

      co_return expected<void>();
    }


  private:
    state_enum_type state_;

    mutable std::mutex state_mutex_;
    std::map<state_enum_type, std::tuple<state_enum_type, vds::async_result<vds::expected<void>>>> state_expectants_;
  };
  
};

#endif//__VDS_CORE_STATE_MACHINE_H_
