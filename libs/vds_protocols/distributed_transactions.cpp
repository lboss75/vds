/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "distributed_transactions.h"
/*
void vds::distributed_transactions::server_code::process_request(request_message request)
{
  if (request.request_number < this->clients_[request.client_id].last_request_number) {
    return;//Drop request
  }

  if (request.request_number == this->clients_[request.client_id].last_request_number) {
    resend(this->clients_[request.client_id]);
    return;
  }

  this->op_number_++;

  log_entry entry;
  entry.op_number = this->op_number_;
  entry.message = request.op;

  this->log_.push(entry);

  //
  prepare_message prepare;
  prepare.current_view_number = this->current_view_number_;
  prepare.client_message = request.op;
  prepare.request_number = this->last_request_number_++;
  prepare.commit_number = this->last_commit_number_;

  this->sent_to_replicas(prepare);
}

void vds::distributed_transactions::server_code::process_prepare(prepare_message prepare)
{
  while (this->last_unbroken_op_number_ + 1 < prepare.request_number) {
    this->requery_log(this->last_unbroken_op_number_ + 1);
  }

  this->log_[prepare.request_number] = prepare.client_message;

  //TODO
  if (this->last_unbroken_op_number_ + 1 == prepare.request_number) {
    this->last_unbroken_op_number_ = prepare.request_number;
  }

  prepare_ok_message prepare_ok;
  prepare_ok.current_view_number = this->current_view_number_;
  prepare_ok.request_number = prepare.request_number;
  prepare_ok.view_id = this->view_id_;

  this->send(prepare_ok);
}

void vds::distributed_transactions::server_code::process_prepare_ok(prepare_ok_message prepare_ok)
{
  this->log_[prepare_ok.request_number].commit_count++;

  if (this->log_[prepare_ok.request_number].commit_count > this->min_commit_count_) {
    reply_message reply;
    reply.current_view_number = this->current_view_number_;
    reply.request_number = this->log_[prepare_ok.request_number].request_number;
    reply.result = "";

    this->last_commit_number_++;

    this->send_to_client(reply_message);
    //Timeout
  }
}

void vds::distributed_transactions::server_code::start_view_change()
{
  start_view_change_message message;
  message.current_view_number = 0;//??
  message.view_id = this->view_id_;

  this->send_to_views(message);
}

void vds::distributed_transactions::server_code::process_start_view_change(
  start_view_change_message start_view_change
)
{
  do_view_change_message message;
  message.current_view_number = this->current_view_number_;
  message.log = 0;//?
  message.last_primary_view = this->current_view_number_;
  message.op_number = 0;//
  message.commit_number = this->last_commit_number_;
}


*/

