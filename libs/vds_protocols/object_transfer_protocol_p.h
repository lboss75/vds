#ifndef __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_P_H_
#define __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "object_transfer_protocol.h"
#include "messages.h"
#include "database_orm.h"
#include "chunk_manager.h"

namespace vds {
  class route_hop;
  class object_request;
  class connection_session;
  class object_offer_replicas;

  class _object_transfer_protocol
  {
  public:
    _object_transfer_protocol();
    ~_object_transfer_protocol();
    
    server_task_manager::task_state
      download_object(
        const service_provider & sp,
        const guid & object_version);

    void on_object_request(
      const service_provider & sp,
      database_transaction & tr,
      const connection_session & session,
      const object_request & message);
    
    void object_offer(
      const service_provider & sp,
      database_transaction & tr,
      const connection_session & session,
      const object_offer_replicas & message);
    
  private:

    //Database
    class download_object_task_table : public database_table
    {
    public:
      download_object_task_table()
        : database_table("download_object_task"),
          object_id(this, "object_id")
      {
      }

      database_column<guid> object_id;
    };
    
  };

  class route_hop
  {
  public:
    route_hop(
      const guid & server_id,
      const std::string & return_address);
    
    const guid & server_id() const { return this->server_id_; }
    const std::string & return_address() const { return this->return_address_; }
    
  private:
    guid server_id_;
    std::string return_address_;
  };
  
  class object_request
  {
  public:
    static const char message_type[];
    static const uint32_t message_type_id = (uint32_t)message_identification::object_request_message_id;
    
    void serialize(binary_serializer & b) const;
    std::shared_ptr<json_value> serialize() const;
    
    object_request(const const_data_buffer & binary_form);

    object_request(
      const guid & server_id,
      uint64_t index,
      const guid & target_storage_id,
      const std::list<ichunk_manager::replica_type> & replicas)
    : server_id_(server_id),
      index_(index),
      target_storage_id_(target_storage_id),
      replicas_(replicas)
    {
    }
    
    const guid & server_id() const { return this->server_id_; }
    uint64_t index() const { return this->index_; }
    const guid & target_storage_id() const { return this->target_storage_id_; }
    const std::list<ichunk_manager::replica_type> & replicas() const { return this->replicas_; }
    
  private:
    guid server_id_;
    uint64_t index_;
    guid target_storage_id_;
    std::list<ichunk_manager::replica_type> replicas_;
  };
  
  class object_offer_replicas
  {
  public:
    static const uint32_t message_type_id = (uint32_t)message_identification::object_offer_replicas_message_id;
    
    void serialize(binary_serializer & b) const;
    
    object_offer_replicas(const const_data_buffer & binary_form);
    
    object_offer_replicas(
      const guid & server_id,
      uint64_t index,
      ichunk_manager::replica_type replica,
      const const_data_buffer & data)
    : server_id_(server_id),
      index_(index),
      replica_(replica),
      data_(data)
    {
    }

    const guid & server_id() const { return this->server_id_; }
    uint64_t index() const { return this->index_; }
    ichunk_manager::replica_type replica() const { return this->replica_; }
    const_data_buffer data() const { return this->data_; }
    
  private:
    guid server_id_;
    uint64_t index_;
    ichunk_manager::replica_type replica_;
    const_data_buffer data_;
  };
  
}

#endif // __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_P_H_
