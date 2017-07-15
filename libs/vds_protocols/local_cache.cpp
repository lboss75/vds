/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "local_cache.h"
#include "local_cache_p.h"
#include "storage_object_id.h"
#include "chunk_manager.h"

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

vds::async_task<vds::server_task_manager::task_state> vds::ilocal_cache::download_object(
  const service_provider & sp,
  const guid & version_id)
{
  return static_cast<_local_cache *>(this)->download_object(sp, version_id);
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

vds::async_task<vds::server_task_manager::task_state> vds::_local_cache::download_object(
  const service_provider & sp,
  const guid & version_id)
{
  return create_async_task([](
    const std::function<void(const service_provider & sp, server_task_manager::task_state state)> & done,
    const error_handler & on_error,
    const service_provider & sp) {
    database_transaction_scope tr(sp, *(*sp.get<iserver_database>())->get_db());
    
    std::list<ichunk_manager::object_chunk_map> object_map;
    sp.get<ichunk_manager>()->get_object_map(sp, tr, version_id, object_map);
    
    for(auto & p : object_map) {
      
    }
    
    tr.commit();
  });
}
