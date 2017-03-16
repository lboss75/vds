#ifndef __VDS_DATA_CHUNK_FILE_H_
#define __VDS_DATA_CHUNK_FILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _chunk_file;
  class _chunk_file_creator;

  class chunk_server
  {
  public:
    chunk_server(
      const guid & server_id, 
      asymmetric_private_key & key,
      const foldername & root_folder
    );

    //
    uint64_t add_data(const void * data, size_t size);
    void add_replica(data_buffer & data);

    void get_local_replicas(const guid & source_id, uint64_t object_id, std::list<uint16_t> & result);

    data_buffer load_data(const guid & source_id, uint64_t object_id, uint16_t replica);

    void subscribe_log(event_handler<const std::string &> & handler);

  private:
  };
}

#endif//__VDS_DATA_CHUNK_FILE_H_
