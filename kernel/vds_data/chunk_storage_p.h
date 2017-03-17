#ifndef __VDS_DATA_CHUNK_STORAGE_P_H_
#define __VDS_DATA_CHUNK_STORAGE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk_storage.h"
#include "chunk.h"

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


  class _chunk_storage
  {
  public:
    _chunk_storage(
      chunk_storage * owner,
      const guid & source_id,
      uint16_t min_horcrux);

    //
    void generate_replica(
      binary_serializer & s,
      uint64_t index,
      uint16_t replica,
      const void * data,
      size_t size);

  private:
    chunk_storage * owner_;
    guid source_id_;
    uint16_t min_horcrux_;

    std::map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
  };

}

#endif//__VDS_DATA_CHUNK_STORAGE_P_H_
