/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_storage.h"
#include "chunk_storage_p.h"

vds::chunk_storage::horcrux::horcrux(binary_deserializer& s)
{
  s >> this->replica_ >> this->size_ >> this->data_;
}

vds::chunk_storage::horcrux::horcrux(binary_deserializer&& s)
{
  s >> this->replica_ >> this->size_>> this->data_;
}

vds::chunk_storage::chunk_storage(uint16_t min_horcrux)
  : impl_(new _chunk_storage(this, min_horcrux))
{
}

vds::chunk_storage::~chunk_storage()
{
  delete this->impl_;
}

void vds::chunk_storage::start()
{
}

void vds::chunk_storage::stop()
{
}

/////////////////////////////////////////////////////////////////////////////////////
vds::ichunk_storage::ichunk_storage(chunk_storage * owner)
  : owner_(owner)
{
}

void vds::ichunk_storage::generate_replica(
  binary_serializer & s,
  uint16_t replica,
  const void * data,
  size_t size)
{
  this->owner_->impl_->generate_replica(s, replica, data, size);
}

void vds::ichunk_storage::restore_data(
  binary_serializer & s,
  const std::list<chunk_storage::horcrux> & chunks)
{
  this->owner_->impl_->restore_data(s, chunks);
}
/////////////////////////////////////////////////////////////////////////////////////
vds::_chunk_storage::_chunk_storage(
  chunk_storage * owner,
  uint16_t min_horcrux)
: owner_(owner),
  min_horcrux_(min_horcrux)
{
}

void vds::_chunk_storage::generate_replica(
  binary_serializer & s,
  uint16_t replica,
  const void * data,
  size_t size)
{
  chunk_generator<uint16_t> * generator;

  auto p = this->generators_.find(replica);
  if (this->generators_.end() == p) {
    generator = new chunk_generator<uint16_t>(this->min_horcrux_, replica);
    this->generators_[replica].reset(generator);
  }
  else {
    generator = p->second.get();
  }

  s << replica << (uint16_t)size;
  generator->write(s, data, size);
}

void vds::_chunk_storage::restore_data(
  binary_serializer & s,
  const std::list<chunk_storage::horcrux> & chunks)
{
  if(this->min_horcrux_ != chunks.size()){
    throw std::runtime_error("Error at restoring data");
  }
  
  const auto size = chunks.begin()->data().size();
  std::vector<uint16_t> replicas;
  std::vector<const const_data_buffer *> datas;
  
  for(auto & p : chunks){
    if(size != p.data().size()){
      throw std::runtime_error("Error at restoring data");
    }
    
    replicas.push_back(p.replica());
    datas.push_back(&p.data());
  }
  
  chunk_restore<uint16_t> restore(this->min_horcrux_, replicas.data());
  restore.restore(s, datas, size);
}

