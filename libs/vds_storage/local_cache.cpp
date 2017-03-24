/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "local_cache.h"
#include "local_cache_p.h"
#include "storage_object_id.h"

vds::local_cache::local_cache(const service_provider& sp)
: impl_(new _local_cache(sp, this))
{
}

vds::local_cache::~local_cache()
{
  delete this->impl_;
}

std::unique_ptr<vds::data_buffer> vds::local_cache::get_object(
  const full_storage_object_id& object_id)
{
  return this->impl_->get_object(object_id);
}

vds::filename vds::local_cache::get_object_filename(const vds::guid& server_id, uint64_t index)
{
  return this->impl_->get_object_filename(server_id, index);
}

////////////////////////////////////////////////////////////
vds::_local_cache::_local_cache(
  const service_provider& sp,
  local_cache * owner)
: owner_(owner),
  root_folder_(foldername(persistence::current_user(sp), ".vds"), "local_cache")
{
  this->root_folder_.create();
}

vds::_local_cache::~_local_cache()
{
}

std::unique_ptr<vds::data_buffer> vds::_local_cache::get_object(
  const full_storage_object_id& object_id)
{
  filename fn(
    foldername(this->root_folder_, object_id.source_server_id().str()),
    std::to_string(object_id.index()));

  
  if(!file::exists(fn)){
    return std::unique_ptr<data_buffer>();
  }
  
  return std::unique_ptr<data_buffer>(
    new data_buffer(
      inflate::inflate_buffer(
        file::read_all(fn))));
}

vds::filename vds::_local_cache::get_object_filename(const vds::guid& server_id, uint64_t index)
{
  foldername folder(this->root_folder_, server_id.str());
  folder.create();
  
  return filename(folder, std::to_string(index));
}
