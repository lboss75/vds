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
    
    
    class horcrux
    {
    public:
      horcrux(binary_deserializer & s);
      horcrux(binary_deserializer && s);
      
      const guid & source_id() const { return this->source_id_; }
      uint64_t index() const { return this->index_; }
      uint16_t replica() const { return this->replica_; }
      uint16_t size() const { return this->size_; }
      const data_buffer & data() const { return this->data_; }
      
    private:
      guid source_id_;
      uint64_t index_;
      uint16_t replica_;
      uint16_t size_;
      data_buffer data_;
    };
    
    void restore_data(
      binary_serializer & s,
      const std::list<horcrux> & chunks);

  private:
    _chunk_storage * impl_;
  };
}

#endif//__VDS_DATA_CHUNK_STORAGE_H_
