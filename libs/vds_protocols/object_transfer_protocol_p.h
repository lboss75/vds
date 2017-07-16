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
      const guid & from_server_id,
      const object_request & message);
    
    void query_data(
      const service_provider & sp,
      database_transaction & tr,
      const vds::guid & server_id,
      ichunk_manager::index_type chunk_index,
      const std::map<guid, std::list<ichunk_manager::replica_type>> & data_request);
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
      const guid & storage_id,
      const std::list<ichunk_manager::replica_type> & replicas)
    : server_id_(server_id),
      index_(index),
      storage_id_(storage_id),
      replicas_(replicas)
    {
    }
    
    const guid & server_id() const { return this->server_id_; }
    uint64_t index() const { return this->index_; }
    const guid & storage_id() const { return this->storage_id_; }
    const std::list<ichunk_manager::replica_type> & replicas() const { return this->replicas_; }
    
  private:
    guid server_id_;
    uint64_t index_;
    guid storage_id_;
    std::list<ichunk_manager::replica_type> replicas_;
  };
  
}

#endif // __VDS_PROTOCOLS_OBJECT_TRANSFER_PROTOCOL_P_H_
