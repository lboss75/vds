/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"
#include "storage_log.h"

vds::async_task<const vds::server_log_new_object &>
vds::ichunk_manager::add(
  const service_provider & sp,
  const const_data_buffer& data)
{
  return static_cast<_chunk_manager *>(this)->add(sp, data);
}

vds::const_data_buffer vds::ichunk_manager::get(
  const service_provider & sp,
  const guid & server_id, uint64_t index)
{
  return static_cast<_chunk_manager *>(this)->get(sp, server_id, index);
}

void vds::ichunk_manager::set_next_index(
  const service_provider & sp,
  uint64_t next_index)
{
  static_cast<_chunk_manager *>(this)->set_next_index(sp, next_index);
}

//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager()
: last_tmp_file_index_(0),
  last_obj_file_index_(0),
  obj_size_(0)
{
}

vds::_chunk_manager::~_chunk_manager()
{
}

vds::async_task<const vds::server_log_new_object &>
vds::_chunk_manager::add(
  const service_provider & sp,
  const const_data_buffer& data)
{
  auto deflated_data = std::make_shared<std::vector<uint8_t>>();
  return create_async_task(
    [data, deflated_data](
      const std::function<void (const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp){
      dataflow(
        dataflow_arguments<uint8_t>(data.data(), data.size()),
        deflate(),
        collect_data(*deflated_data)
      )(
        done,
        on_error,
        sp);
    })
  .then(
    [this,
    sp,
    deflated_data,
    original_lenght = data.size(),
    original_hash = hash::signature(hash::sha256(), data)
    ](
      const std::function<void(const service_provider & sp, const vds::server_log_new_object &)> & done,
      const error_handler & on_error,
      const service_provider & sp) {
      
        this->tmp_folder_mutex_.lock();
        auto tmp_index = this->last_tmp_file_index_++;
        this->tmp_folder_mutex_.unlock();

        filename fn(this->tmp_folder_, std::to_string(tmp_index));
        file f(fn, file::create_new);
        f.write(deflated_data->data(), deflated_data->size());
        f.close();

        this->obj_folder_mutex_.lock();
        auto index = this->last_obj_file_index_++;
        this->obj_size_ += deflated_data->size();
        this->obj_folder_mutex_.unlock();

        file::move(fn,
          sp.get<ilocal_cache>()->get_object_filename(
            sp, sp.get<istorage_log>()->current_server_id(), index));

        this->obj_folder_mutex_.lock();
        if (max_obj_size_ < this->obj_size_) {
          this->generate_chunk(sp);
        }
        this->obj_folder_mutex_.unlock();

        auto result = server_log_new_object(
          index,
          original_lenght,
          std::move(original_hash),
          deflated_data->size(),
          hash::signature(hash::sha256(), deflated_data->data(), deflated_data->size()));

        sp.get<istorage_log>()->add_to_local_log(sp, result.serialize().get());
        done(sp, result);
      });
}

vds::const_data_buffer vds::_chunk_manager::get(
  const service_provider & sp,
  const guid & server_id,
  uint64_t index)
{
  return inflate::inflate_buffer(
    file::read_all(sp.get<ilocal_cache>()->get_object_filename(sp, server_id, index)));
}

void vds::_chunk_manager::generate_chunk(const service_provider & sp)
{
  throw std::runtime_error("Not implemented");
}

void vds::_chunk_manager::set_next_index(const service_provider & sp, uint64_t next_index)
{
  this->last_obj_file_index_ = next_index;
}

void vds::_chunk_manager::start(const service_provider & sp)
{
  this->tmp_folder_ = foldername(foldername(persistence::current_user(sp), ".vds"), "tmp");
  this->tmp_folder_.create();

  this->set_next_index(
    sp,
    sp.get<iserver_database>()->last_object_index(
      sp,
      sp.get<istorage_log>()->current_server_id()));
}

void vds::_chunk_manager::stop(const service_provider & sp)
{
}

