/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "vsr_protocol.h"

vds::vsr_protocol::client::client(const service_provider & sp)
: sp_(sp),
  log_(sp, "VSR client"),
  last_request_number_(0),
  client_id_(0),
  current_primary_view_(0),
  server_count_(0),
  task_manager_(sp.get<itask_manager>()),
  server_queue_(sp.get<iserver_queue>())
{
}

void vds::vsr_protocol::client::start()
{
  this->server_queue_.new_client();
}

void vds::vsr_protocol::client::new_client_request_complete(const vsr_new_client_message_complete & response)
{
  std::unique_lock<std::mutex> lock(this->lock_mutex_);

  if (0 == this->client_id_ || response.server_count > this->server_count_) {
    this->client_id_ = response.client_id;
    this->current_primary_view_ = response.current_primary_view;
    this->server_count_ = response.server_count;

    this->client_id_assigned();
  }
}

void vds::vsr_protocol::client::client_id_assigned()
{
}

vds::vsr_protocol::server::server(const service_provider & sp)
: client(sp),
  log_(sp, "VSR server"),
  get_client_id_timeout_(std::bind(&server::get_client_id_timeout, this)),
  last_request_number_(0),
  last_commit_number_(0)
{
}

void vds::vsr_protocol::server::start()
{
  this->get_client_id_task_ = this->task_manager_.create_job(this->get_client_id_timeout_);
}

void vds::vsr_protocol::server::new_client()
{
}

void vds::vsr_protocol::server::get_client_id_timeout()
{
  std::unique_lock<std::mutex> lock(this->lock_mutex_);

  if (0 == this->client_id_) {
    this->log_.warning("I am the first server!!!");

    this->client_id_ = 1;
    this->current_primary_view_ = this->client_id_;
    this->server_count_ = 1;

    this->client_id_assigned();
  }
}