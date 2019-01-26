/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_storage.h"
#include "private/chunk_storage_p.h"

vds::chunk_storage::chunk_storage(uint16_t min_horcrux)
  : impl_(new _chunk_storage(min_horcrux))
{
}

vds::chunk_storage::~chunk_storage()
{
  delete this->impl_;
}


vds::expected<vds::const_data_buffer> vds::chunk_storage::generate_replica(
  uint16_t replica,
  const void * data,
  size_t size)
{
  return this->impl_->generate_replica(replica, data, size);
}

vds::expected<vds::const_data_buffer> vds::chunk_storage::restore_data(
  const std::unordered_map<uint16_t, const_data_buffer> & horcruxes)
{
  return this->impl_->restore_data(horcruxes);
}
/////////////////////////////////////////////////////////////////////////////////////
vds::_chunk_storage::_chunk_storage(
  uint16_t min_horcrux)
: min_horcrux_(min_horcrux)
{
}

vds::expected<vds::const_data_buffer> vds::_chunk_storage::generate_replica(
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

  binary_serializer s;
  CHECK_EXPECTED(generator->write(s, data, size));
  return s.move_data();
}

vds::expected<vds::const_data_buffer> vds::_chunk_storage::restore_data(
  const std::unordered_map<uint16_t, const_data_buffer> & horcruxes)
{
  if(this->min_horcrux_ != horcruxes.size()){
    return vds::make_unexpected<std::runtime_error>("Error at restoring data");
  }

  auto size = horcruxes.begin()->second.size();
  
  std::vector<uint16_t> replicas;
  std::vector<const_data_buffer> datas;
  
  for(auto & p : horcruxes){
    if (size != p.second.size()) {
      return vds::make_unexpected<std::runtime_error>("Error at restoring data");
    }

    replicas.push_back(p.first);
    datas.push_back(p.second);
  }
  
  chunk_restore<uint16_t> restore(this->min_horcrux_, replicas.data());
  
  return restore.restore(datas);
}

