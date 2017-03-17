#ifndef __VDS_DATA_CHUNK_FILE_P_H_
#define __VDS_DATA_CHUNK_FILE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_file.h"

namespace vds {
  class _chunk_file
  {
  public:
    _chunk_file(
      const guid & source_id,
      const uint64_t & index,
      const uint16_t & replica,
      const data_buffer & data);

    _chunk_file(binary_deserializer & s);

    binary_serializer & serialize(binary_serializer & s);

  private:
    guid source_id_;
    uint64_t index_;
    uint16_t replica_;
    data_buffer data_;
  };

  struct _chunk_log_file
  {
    guid record_id;
    std::vector<guid> parents;

    guid source_id;
    std::string data;

    void read(
      file & fn,
      const asymmetric_public_key & key);

    void write(
      file & fn,
      const asymmetric_private_key & key);
  };


  class _chunk_server
  {
  public:
    _chunk_server(
      const guid & source_id);

    //
    uint64_t generate_replica(binary_deserializer & s, size_t replica, const void * data, size_t size);

    void get_local_replicas(const guid & source_id, uint64_t object_id, std::list<uint16_t> & result);

    data_buffer load_data(const guid & source_id, uint64_t object_id, uint16_t replica);

    void subscribe_log(event_handler<const std::string &> & handler);

  private:
    guid server_id_;
    certificate cert_;
    asymmetric_private_key key_;
    foldername root_folder_;

    foldername data_folder_;

    uint64_t last_chunk_file_;
  };

}

#endif//__VDS_DATA_CHUNK_FILE_P_H_
