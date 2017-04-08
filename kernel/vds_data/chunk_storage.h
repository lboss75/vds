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

    void start();
    void stop();

    
    class horcrux
    {
    public:
      horcrux(binary_deserializer & s);
      horcrux(binary_deserializer && s);
      
      const guid & source_id() const { return this->source_id_; }
      uint64_t index() const { return this->index_; }
      uint16_t replica() const { return this->replica_; }
      uint16_t size() const { return this->size_; }
      const const_data_buffer & data() const { return this->data_; }
      
    private:
      guid source_id_;
      uint64_t index_;
      uint16_t replica_;
      uint16_t size_;
      const_data_buffer data_;
    };
   

  private:
    friend class ichunk_storage;

    _chunk_storage * const impl_;
  };

  class ichunk_storage
  {
  public:
    ichunk_storage(chunk_storage * owner);

    void generate_replica(
      binary_serializer & s,
      uint64_t index,
      uint16_t replica,
      const void * data,
      size_t size);

    void restore_data(
      binary_serializer & s,
      const std::list<chunk_storage::horcrux> & chunks);

  private:
    chunk_storage * const owner_;
  };
}

#endif//__VDS_DATA_CHUNK_STORAGE_H_
