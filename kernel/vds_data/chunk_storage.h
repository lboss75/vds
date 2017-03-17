#ifndef __VDS_DATA_CHUNK_STORAGE_H_
#define __VDS_DATA_CHUNK_STORAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _chunk_storage;

  class chunk_storage
  {
  public:
    chunk_storage(
      const guid & source_id,
      uint16_t min_horcrux
      );
    ~chunk_storage();

    void generate_replica(
      binary_serializer & s,
      uint64_t index,
      uint16_t replica,
      const void * data,
      size_t size);

  private:
    _chunk_storage * impl_;
  };
}

#endif//__VDS_DATA_CHUNK_STORAGE_H_
