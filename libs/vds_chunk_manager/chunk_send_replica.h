#ifndef __VDS_CHUNK_MANAGER_CHUNK_SEND_REPLICA_H_
#define __VDS_CHUNK_MANAGER_CHUNK_SEND_REPLICA_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class chunk_send_replica {
  public:
    chunk_send_replica(
        const guid & source_node_id,
        const const_data_buffer & body)
    : source_node_id_(source_node_id),
      body_(body){
    }

    const_data_buffer serialize() const {
      binary_serializer s;
      s << this->source_node_id_ << this->body_;
      return s.data();
    }

  private:
    guid source_node_id_;
    const_data_buffer body_;

  };
}

#endif //__VDS_CHUNK_MANAGER_CHUNK_SEND_REPLICA_H_
