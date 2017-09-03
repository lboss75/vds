#ifndef __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_
#define __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_log_logic.h"
#include "server_database.h"
#include "chunk_manager_p.h"
#include "principal_manager_p.h"

namespace vds {
  class _server_log_logic
  {
  public:
    _server_log_logic(
      const service_provider & sp,
      database_transaction & tr)
    : sp_(sp), tr_(tr)
    {
    }
    
    void operator()(const principal_log_new_object & msg) const
    {
      this->sp_.get<iserver_database>()->add_object(this->sp_, this->tr_, msg);
    }
    
    void operator()(const principal_log_new_chunk & msg) const
    {
      auto chunk_manager = this->sp_.get<ichunk_manager>();
      
      (*chunk_manager)->add_chunk(
        this->sp_,
        this->tr_,
        msg.server_id(),
        msg.chunk_index(),
        msg.object_id(),
        msg.chunk_size(),
        msg.chunk_hash());
    }
    
    void operator()(const principal_log_new_replica & msg) const
    {
      auto chunk_manager = this->sp_.get<ichunk_manager>();
      
      (*chunk_manager)->add_chunk_replica(
        this->sp_,
        this->tr_,
        msg.server_id(),
        msg.chunk_index(),
        msg.index(),
        msg.replica_size(),
        msg.replica_hash());
      
      (*chunk_manager)->add_chunk_store(
        this->sp_,
        this->tr_,
        msg.server_id(),
        msg.chunk_index(),
        msg.index(),
        msg.server_id());
    }
    
    void operator()(const server_log_root_certificate & msg) const
    {
      auto manager = this->sp_.get<principal_manager>();
      (*manager)->add_user_principal(
        this->sp_,
        this->tr_,
        "root",
        vds::principal_record(
          msg.id(),
          msg.id(),
          msg.user_cert(),
          msg.user_private_key(),
          msg.password_hash()));
    }
    
    void operator()(const server_log_new_server & msg) const
    {
      auto manager = this->sp_.get<principal_manager>();
      (*manager)->add_principal(
        this->sp_,
        this->tr_,
        vds::principal_record(
          msg.parent_id(),
          msg.id(),
          msg.server_cert(),
          msg.server_private_key(),
          msg.password_hash()));
    }
    
    void operator()(const server_log_new_endpoint & msg) const
    {
      this->sp_.get<iserver_database>()->add_endpoint(
        this->sp_,
        this->tr_,
        msg.server_id().str(),
        msg.addresses());
    }
    
  private:
    service_provider sp_;
    database_transaction & tr_;
  };
}

#endif // __VDS_PROTOCOLS_SERVER_LOG_LOGIC_P_H_
