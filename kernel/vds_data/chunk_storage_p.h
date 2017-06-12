#ifndef __VDS_DATA_CHUNK_STORAGE_P_H_
#define __VDS_DATA_CHUNK_STORAGE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "chunk.h"
#include "chunk_storage.h"

namespace vds {
  class _chunk_storage
  {
  public:
    _chunk_storage(
      chunk_storage * owner,
      uint16_t min_horcrux);

    //
    void generate_replica(
      binary_serializer & s,
      uint16_t replica,
      const void * data,
      size_t size);
    
    void restore_data(
      binary_serializer & s,
      const std::list<chunk_storage::horcrux> & chunks);

  private:
    chunk_storage * owner_;
    uint16_t min_horcrux_;

    std::unordered_map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
  };

}

#endif//__VDS_DATA_CHUNK_STORAGE_P_H_
