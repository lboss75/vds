/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_storage.h"
#include "chunk_storage_p.h"

vds::_chunk_file::_chunk_file(
  const guid & source_id,
  const uint64_t & index,
  const uint16_t & replica,
  const data_buffer & data)
  : source_id_(source_id),
  index_(index),
  replica_(replica),
  data_(data)
{
}

vds::_chunk_file::_chunk_file(binary_deserializer & s)
{
  s >> this->source_id_ >> this->index_ >> this->replica_ >> this->data_;
}

vds::binary_serializer & vds::_chunk_file::serialize(binary_serializer & s)
{
  return s << this->source_id_ << this->index_ << this->replica_ << this->data_;
}

vds::chunk_storage::chunk_storage(const guid & source_id, uint16_t min_horcrux)
  : impl_(new _chunk_storage(this, source_id, min_horcrux))
{
}

vds::chunk_storage::~chunk_storage()
{
  delete this->impl_;
}

void vds::chunk_storage::generate_replica(
  binary_serializer & s,
  uint64_t index,
  uint16_t replica,
  const void * data,
  size_t size)
{
  this->impl_->generate_replica(s, index, replica, data, size);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
vds::_chunk_storage::_chunk_storage(
  chunk_storage * owner,
  const guid & source_id,
  uint16_t min_horcrux)
  : owner_(owner),
  source_id_(source_id),
  min_horcrux_(min_horcrux)
{
}

void vds::_chunk_storage::generate_replica(
  binary_serializer & s,
  uint64_t index,
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

  s << this->source_id_ << index << replica;
  generator->write(s, data, size);
}

