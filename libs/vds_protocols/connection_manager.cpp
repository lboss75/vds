/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "connection_manager.h"
#include "connection_manager_p.h"

vds::connection_manager::connection_manager(
  const service_provider & sp,
  const std::string & from_address)
  : impl_(new _connection_manager(sp, this, from_address))
{
}

vds::connection_manager::~connection_manager()
{
  delete this->impl_;
}

void vds::connection_manager::ready_to_get_messages(iconnection_channel * target)
{
  this->impl_->ready_to_get_messages(target);
}

void vds::connection_manager::send(const std::list<std::string>& to_address, const std::string & body)
{
  this->impl_->send(to_address, body);
}

void vds::connection_manager::broadcast(const std::string & body)
{
  this->impl_->broadcast(body);
}

vds::_connection_manager::_connection_manager(
  const service_provider & sp,
  connection_manager * owner,
  const std::string & from_address)
  : sp_(sp),
  owner_(owner),
  from_address_(from_address)
{
}

vds::_connection_manager::~_connection_manager()
{
}

void vds::_connection_manager::ready_to_get_messages(iconnection_channel * target)
{
  std::lock_guard<std::mutex> lock(this->connection_channels_mutex_);
  this->connection_channels_.push_back(target);
}

void vds::_connection_manager::send(const std::list<std::string>& to_address, const std::string & body)
{
  std::lock_guard<std::mutex> messages_lock(this->messages_mutex_);
  auto need_to_run = this->messages_.empty();
  this->messages_.push_back(connection_message {false, this->from_address_, to_address, body } );

  if (need_to_run) {
    this->sp_.get<imt_service>().async(&_connection_manager::work_thread, this);
  }
}

void vds::_connection_manager::broadcast(const std::string & body)
{
  std::lock_guard<std::mutex> messages_lock(this->messages_mutex_);
  auto need_to_run = this->messages_.empty();

  this->messages_.push_back(connection_message{ true, this->from_address_, std::list<std::string>(), body });

  if (need_to_run) {
    this->sp_.get<imt_service>().async(&_connection_manager::work_thread, this);
  }
}

void vds::_connection_manager::work_thread()
{
  while(!this->sp_.get_shutdown_event().is_shuting_down()){
    bool sent_messages = false;

    {
      std::lock_guard<std::mutex> messages_lock(this->messages_mutex_);
      if (this->messages_.empty()) {
        return;
      }

      std::map<iconnection_channel *, std::map<std::string, size_t>> metrics;
      std::lock_guard<std::mutex> lock(this->connection_channels_mutex_);
      for (auto& c : this->connection_channels_) {
        c->get_delivery_metrics(metrics[c]);
      }

      for (auto mp = this->messages_.begin(); mp != this->messages_.end();) {
        iconnection_channel * best_channel = nullptr;
        size_t best_sum_metric;
        size_t best_wrong_address;
        for (auto & metric : metrics) {
          size_t sum_metric = 0;
          size_t wrong_address = 0;
          for (auto& mt : mp->to_address) {
            auto p = metric.second.find(mt);
            if (metric.second.end() != p) {
              sum_metric += p->second;
            }
            else {
              ++wrong_address;
            }
          }

          if (nullptr == best_channel
            || best_wrong_address > wrong_address
            || best_sum_metric > sum_metric) {
            best_channel = metric.first;
            best_sum_metric = sum_metric;
            best_wrong_address = wrong_address;
          }
        }

        if (nullptr != best_channel) {
          sent_messages = true;
          best_channel->send(mp->from_address, mp->to_address, mp->body);
        }

        if (mp->to_address.empty()) {
          mp = this->messages_.erase(mp);
        }
        else {
          ++mp;
        }
      }
    }

    if (!sent_messages) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

