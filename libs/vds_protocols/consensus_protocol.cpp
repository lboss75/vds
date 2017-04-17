/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_protocol.h"
#include "consensus_protocol_p.h"
#include "node.h"
#include "consensus_messages.h"
#include "connection_manager.h"

vds::consensus_protocol::server::server(
  const service_provider & sp,
  certificate & certificate,
  asymmetric_private_key & private_key,
  connection_manager & connection_manager)
  : impl_(new _server(sp, this, certificate, private_key, connection_manager))
{
}


vds::consensus_protocol::server::~server()
{
  delete this->impl_;
}

void vds::consensus_protocol::server::start()
{
  this->impl_->start();
}

void vds::consensus_protocol::server::stop()
{
  this->impl_->stop();
}

vds::async_task<const vds::json_value *>
vds::consensus_protocol::server::process(
  const service_provider & scope,
  const vds::consensus_messages::consensus_message_who_is_leader & message)
{
  return this->impl_->process(scope, message);
}
///////////////////////////////////////////////////////////////////////////////
vds::consensus_protocol::_server::_server(
  const service_provider & sp,
  server * owner,
  certificate & certificate,
  asymmetric_private_key & private_key,
  connection_manager & connection_manager)
  : sp_(sp),
  log_(sp, "Consensus Server"),
  owner_(owner),
  certificate_(certificate),
  private_key_(private_key),
  connection_manager_(connection_manager),
  check_leader_task_job_(std::bind(&_server::leader_check, this)),
  state_(none),
  leader_check_timer_(0)
{
}

void vds::consensus_protocol::_server::start()
{/*
  storage_cursor<node> node_reader(this->sp_.get<istorage>());
  while (node_reader.read()) {
    this->nodes_[node_reader.current().id()] = { };
  }

  this->connection_manager_.broadcast(
    consensus_messages::consensus_message_who_is_leader(this->certificate_.fingerprint()));

  this->sp_.get<itask_manager>().wait_for(std::chrono::seconds(1)) += this->check_leader_task_job_;
  */
}

void vds::consensus_protocol::_server::stop()
{
}


vds::async_task<const vds::json_value *>
vds::consensus_protocol::_server::process(
  const service_provider & /*scope*/,
  const consensus_messages::consensus_message_who_is_leader & /*message*/)
{
  throw std::runtime_error("Not implemented");
  /*
  switch (this->state_) {
  case leader:
    result.add(consensus_messages::consensus_message_current_leader(this->certificate_.fingerprint()).serialize());
    break;

  case follower:
    result.add(consensus_messages::consensus_message_current_leader(this->leader_).serialize());
    break;
    
  case none:
  case candidate:
    break;
  }*/
}

void vds::consensus_protocol::_server::leader_check()
{
  switch (this->state_) {
  case none:

    if (2 < this->leader_check_timer_++) {
      this->state_ = candidate;
      this->leader_check_timer_ = 0;
//       this->connection_manager_.broadcast(
//         consensus_messages::consensus_message_leader_candidate(this->certificate_.fingerprint()));
    }
    break;

  case candidate:
    if (2 < this->leader_check_timer_++) {
      std::unique_lock<std::mutex> lock(this->messages_to_lead_mutex_);
      this->state_ = leader;
      this->leader_check_timer_ = 0;
      this->become_leader();
//       this->connection_manager_.broadcast(
//         consensus_messages::consensus_message_new_leader(this->certificate_.fingerprint()));
    }
    break;

  case leader:
    break;

  case follower:
    break;

  default:
    throw new std::runtime_error("Invalid consensus protocol state");
  }
  
  this->sp_.get<itask_manager>().wait_for(std::chrono::seconds(1)) += this->check_leader_task_job_;
}

void vds::consensus_protocol::_server::become_leader()
{
  this->sp_.get<imt_service>().async([this]() {
    this->flush_messages_to_lead();
  });
}

void vds::consensus_protocol::_server::flush_messages_to_lead()
{
  //std::unique_ptr<server_log_batch> batch(
  //  new server_log_batch(
  //    this->sp_.get<istorage>()
  //    .get_storage_log()
  //    .new_message_id()));

  //{
  //  std::unique_lock<std::mutex> lock(this->messages_to_lead_mutex_);

  //  if (this->messages_to_lead_.empty()) {
  //    return;
  //  }

  //  for (auto& message : this->messages_to_lead_) {
  //    batch->add(std::move(message));
  //  }

  //  this->messages_to_lead_.clear();
  //}

  //auto message_body = batch->serialize()->str();

  //hash h(hash::sha256());
  //h.update(message_body.c_str(), message_body.length());
  //h.final();

  //asymmetric_sign s(hash::sha256(), this->private_key_);
  //s.update(h.signature().data(), h.signature().size());
  //s.final();

  //server_log_record record(std::move(batch));
  //record.add_signature(this->certificate_.subject(), s.signature());

  //this->sp_.get<istorage>().get_storage_log().add_record(record.serialize(false)->str());
}
