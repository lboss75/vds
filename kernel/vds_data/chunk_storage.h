#ifndef __VDS_DATA_CHUNK_STORAGE_H_
#define __VDS_DATA_CHUNK_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>

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


    
    class horcrux
    {
    public:
      horcrux(binary_deserializer & s);
      horcrux(binary_deserializer && s);
      
      uint16_t replica() const { return this->replica_; }
      uint16_t size() const { return this->size_; }
      const const_data_buffer & data() const { return this->data_; }
      
    private:
      uint64_t index_;
      uint16_t replica_;
      uint16_t size_;
      const_data_buffer data_;
    };
   
    const_data_buffer generate_replica(
      uint16_t replica,
      const void * data,
      size_t size);

    void restore_data(
      binary_serializer & s,
      const std::list<chunk_storage::horcrux> & chunks);

  private:
    friend class ichunk_storage;

    _chunk_storage * const impl_;
  };
}

#endif//__VDS_DATA_CHUNK_STORAGE_H_
