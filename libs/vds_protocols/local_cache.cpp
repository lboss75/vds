/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "local_cache.h"
#include "local_cache_p.h"
#include "storage_object_id.h"

std::unique_ptr<vds::const_data_buffer> vds::ilocal_cache::get_object(
  const service_provider& sp,
  const full_storage_object_id& object_id)
{
  return static_cast<_local_cache *>(this)->get_object(sp, object_id);
}

vds::filename vds::ilocal_cache::get_object_filename(
  const service_provider& sp, 
  const vds::guid& server_id, uint64_t index)
{
  return static_cast<_local_cache *>(this)->get_object_filename(sp, server_id, index);
}

////////////////////////////////////////////////////////////
vds::_local_cache::_local_cache()
{
}

vds::_local_cache::~_local_cache()
{
}

void vds::_local_cache::start(const service_provider& sp)
{
  this->root_folder_ = foldername(foldername(persistence::current_user(sp), ".vds"), "local_cache");
  this->root_folder_.create();
}

void vds::_local_cache::stop(const service_provider& sp)
{
}

vds::filename vds::_local_cache::get_object_filename(
  const service_provider& sp, 
  const vds::guid& server_id,
  uint64_t index)
{
  foldername folder(this->root_folder_, server_id.str());
  folder.create();
  
  return filename(folder, std::to_string(index));
}
