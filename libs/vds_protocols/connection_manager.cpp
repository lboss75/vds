/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "connection_manager.h"
#include "connection_manager_p.h"

void vds::_connection_manager::work_thread()
{
  while(!this->sp_.get_shutdown_event().is_shuting_down()){
    std::lock_guard<std::mutex> messages_lock(this->messages_mutex_);
    if(!this->messages_cond_.wait_for(
      messages_lock,
      std::chrono::seconds(1),
      [this]()->bool { return !this->messages_.empty(); })){
      continue;
    }
    
    std::map<std::string, target_metric> metrics;
    for(auto & m : this->messages_){
      for(auto & t : m.to_address){
        auto p = metrics.find(t);
        if(metrics.end() == p){
          metrics[t] = { nullptr, 0};
        }
      }
    }
    
    std::lock_guard<std::mutex> lock(this->connection_channels_mutex_);
    for(auto& c : this->connection_channels_) {
      c.update_target_metrics(metrics);
    }
    
    for(auto mp = this->messages_.begin(); mp != this->messages_.end();){
      for(auto mt = mp->to_address.begin(); mt != mp->to_address.end();){
        auto p = metrics.find(*mt);
        if(metrics.end() != p){
          p->second->channel->send(m.from_address, *mt, m.body);
          mt = mp->to_address.remove(mt);
        }
        else {
          ++mt;
        }
      }
      
      if(mp->to_address.empty()){
        mp = this->messages_.remove(mp);
      }
      else {
        ++mp;
      }
    }
  }
}

