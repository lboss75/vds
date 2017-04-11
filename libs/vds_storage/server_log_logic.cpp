/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "server_log_logic.h"
#include "server_log_logic_p.h"

void vds::_server_log_logic::add_record(const server_log_record & record)
{
  if (!this->db_.get(this->sp_).save_record(record)) {
    return;
  }

  for (auto& p : record.parents()) {
    if (!this->db_.get(this->sp_).have_processed_log_record(p)) {
      return;//parent don't exists or not processed
    }
  }

  this->process_record(record);

  for (auto& p : record.parents()) {
    this->db_.get(this->sp_).remove_tail_flag_log_record(p);
  }

  std::list<server_log_record::record_id> followers;
  this->db_.get(this->sp_).get_log_record_unprocessed_followers(record.id(), followers);


  if (record.parents().size() == 1 && record.parents().begin() == this->current_id_) {
    //Move head: ... N -> N + 1
    this->current_id_ = record.id();
  }
  else if(!db.exist(record)){

    // (not exist N, not exist M): not exist ID -> just save
    for (auto parent : record.parents()) {
      if (!db.exists(parent)) {
        this->owner_->record_not_found(parent);
      }
      else {

      }
    }

    // (not exist N, not exist M): exist ID -> just save
    
    // (not exist N, exist M): not exist ID ->

    // (not exist N, exist M): exist ID ->

    // (exist N, not exist M): not exist ID ->

    // (exist N, not exist M): exist ID ->

    // (exist N, exist M): not exist ID ->

    // (exist N, exist M): exist ID -> merge

  }
  
}