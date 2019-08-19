#ifndef __VDS_SERVER_SERVER_STATISTIC_H_
#define __VDS_SERVER_SERVER_STATISTIC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "route_statistic.h"
#include "sync_statistic.h"
#include "session_statistic.h"

namespace vds {

  struct server_statistic {
    size_t db_queue_length_;
    foldername current_user_;
    foldername local_machine_;
    sync_statistic sync_statistic_;
    route_statistic route_statistic_;
    session_statistic session_statistic_;
    std::shared_ptr<vds::json_value> serialize() const {
      auto result = std::make_shared<vds::json_object>();
      result->add_property("db_queue_length", std::to_string(this->db_queue_length_));
      result->add_property("current_user_folder", this->current_user_.full_name());
      result->add_property("local_machine_folder", this->local_machine_.full_name());
      result->add_property("sync", this->sync_statistic_.serialize());
      result->add_property("route", this->route_statistic_.serialize());
      result->add_property("session", this->session_statistic_.serialize());
      return result;
    }
  };

}
#endif //__VDS_SERVER_SERVER_STATISTIC_H_
