#ifndef __VDS_DATA_CHUNK_FILE_P_H_
#define __VDS_DATA_CHUNK_FILE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_file.h"

namespace vds {
  struct _chunk_file
  {
    guid source_id;
    uint64_t index;
    data_buffer data;

    void read(
      const filename & fn,
      const asymmetric_public_key & key);

    void write(
      const filename & fn,
      const asymmetric_private_key & key);

  };

  struct _chunk_replica_file
  {
    guid source_id;
    uint64_t index;
    data_buffer data;

    uint16_t replica;
    guid signer_id;
    data_buffer data_sign;
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
    uint64_t add_data(const void * data, size_t size);
    void add_replica(data_buffer & data);

    void get_local_replicas(const guid & source_id, uint64_t object_id, std::list<uint16_t> & result);

    data_buffer load_data(const guid & source_id, uint64_t object_id, uint16_t replica);

    void subscribe_log(event_handler<const std::string &> & handler);

  private:
    guid server_id_;
    certificate cert_;
    asymmetric_private_key key_;
    foldername root_folder_;

    foldername data_folder_;

    _chunk_file last_chunk_file_;
  };

}

#endif//__VDS_DATA_CHUNK_FILE_P_H_
