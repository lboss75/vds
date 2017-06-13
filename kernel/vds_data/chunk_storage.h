#ifndef __VDS_DATA_CHUNK_STORAGE_H_
#define __VDS_DATA_CHUNK_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <unordered_map>

#include "guid.h"
#include "binary_serialize.h"

namespace vds {
  class _chunk_storage;
  
  class chunk_storage
  {
  public:
    chunk_storage(
      uint16_t min_horcrux
      );
    ~chunk_storage();
    
    const_data_buffer generate_replica(
      uint16_t replica,
      const void * data,
      size_t size);

    const_data_buffer restore_data(
      const std::unordered_map<uint16_t, const_data_buffer> & horcruxes);

  private:
    friend class ichunk_storage;

    _chunk_storage * const impl_;
  };
}

#endif//__VDS_DATA_CHUNK_STORAGE_H_
