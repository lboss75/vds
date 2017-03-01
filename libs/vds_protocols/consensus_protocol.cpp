/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "consensus_protocol.h"
#include "consensus_protocol_p.h"
#include "node.h"
#include "storage_service.h"

vds::consensus_protocol::server::server(
  const service_provider & sp,
  storage_log & storage,
  certificate & certificate,
  asymmetric_private_key & private_key)
  : impl_(new _server(sp, storage, this, certificate, private_key))
{
}


vds::consensus_protocol::server::~server()
{
}

void vds::consensus_protocol::server::start()
{
  this->impl_->start();
}

void vds::consensus_protocol::server::stop()
{
  this->impl_->stop();
}

void vds::consensus_protocol::server::register_server(const std::string & certificate_body)
{
  this->impl_->register_server(certificate_body);
}

///////////////////////////////////////////////////////////////////////////////
vds::consensus_protocol::_server::_server(
  const service_provider & sp,
  storage_log & storage,
  server * owner,
  certificate & certificate,
  asymmetric_private_key & private_key)
  : sp_(sp),
  log_(sp, "Consensus Server"),
  storage_(storage),
  owner_(owner),
  certificate_(certificate),
  private_key_(private_key),
  check_leader_task_job_(sp.get<itask_manager>().create_job("Leader checking", this, &_server::leader_check)),
  state_(none),
  leader_check_timer_(0)
{
}

void vds::consensus_protocol::_server::start()
{
  storage_cursor<node> node_reader(this->sp_.get<istorage>());
  while (node_reader.read()) {
    this->nodes_[node_reader.current().id()] = { };
  }

  this->check_leader_task_job_.schedule(std::chrono::system_clock::now() + std::chrono::seconds(1));

}

void vds::consensus_protocol::_server::stop()
{
  this->check_leader_task_job_.destroy();
}

void vds::consensus_protocol::_server::register_server(const std::string & certificate_body)
{

  std::unique_lock<std::mutex> lock(this->messages_to_lead_mutex_);
  this->messages_to_lead_.push_back(server_log_new_server(certificate_body).serialize());

  if (leader == this->state_) {
    this->sp_.get<imt_service>().async([this]() {
      this->flush_messages_to_lead();
    });
  }
}

void vds::consensus_protocol::_server::leader_check()
{
  switch (this->state_) {
  case none:
    if (this->leader_check_timer_++ > 2) {
      this->state_ = candidate;
      this->leader_check_timer_ = 0;
    }
    break;

  case candidate:
    if (this->leader_check_timer_++ > 2) {
      std::unique_lock<std::mutex> lock(this->messages_to_lead_mutex_);
      this->state_ = leader;
      this->leader_check_timer_ = 0;
      this->become_leader();
    }
    break;

  case leader:
    break;

  case follower:
    break;

  default:
    throw new std::runtime_error("Invalid consensus protocol state");
  }
  
  this->check_leader_task_job_.schedule(std::chrono::system_clock::now() + std::chrono::seconds(1));
}

void vds::consensus_protocol::_server::become_leader()
{
  this->sp_.get<imt_service>().async([this]() {
    this->flush_messages_to_lead();
  });
}

void vds::consensus_protocol::_server::flush_messages_to_lead()
{
  server_log_batch batch;
  batch.messages_.reset(new json_array());

  {
    std::unique_lock<std::mutex> lock(this->messages_to_lead_mutex_);

    if (this->messages_to_lead_.empty()) {
      return;
    }

    for (auto& message : this->messages_to_lead_) {
      batch.messages_->add(std::move(message));
    }

    this->messages_to_lead_.clear();
  }
  batch.message_id_ = this->storage_.new_message_id();

  std::unique_ptr<json_value> m(batch.serialize());

  auto message_body = m->str();

  hash h(hash::sha256());
  h.update(message_body.c_str(), message_body.length());
  h.final();

  asymmetric_sign s(hash::sha256(), this->private_key_);
  s.update(h.signature(), h.signature_length());
  s.final();

  server_log_record record;
  record.fingerprint_ = this->certificate_.fingerprint();
  record.signature_ = base64::from_bytes(s.signature(), s.signature_length());
  record.message_ = std::move(m);

  this->storage_.add_record(record.serialize()->str());
}
