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
      uint16_t min_horcrux);

    expected<const_data_buffer> generate_replica(
      uint16_t replica,
      const void * data,
      size_t size);

    expected<const_data_buffer> restore_data(
      const std::unordered_map<uint16_t, const_data_buffer> & horcruxes);

  private:
    uint16_t min_horcrux_;

    std::unordered_map<uint16_t, std::unique_ptr<chunk_generator<uint16_t>>> generators_;
  };

}

#endif//__VDS_DATA_CHUNK_STORAGE_P_H_
