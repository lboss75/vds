/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"
#include "storage_log.h"

vds::chunk_manager::chunk_manager(const service_provider & sp)
  : impl_(new _chunk_manager(sp, this))
{
}

vds::chunk_manager::~chunk_manager()
{
  delete this->impl_;
}

void vds::chunk_manager::start()
{
  this->impl_->start();
}

void vds::chunk_manager::stop()
{
  this->impl_->stop();
}

vds::ichunk_manager::ichunk_manager(chunk_manager * owner)
  : owner_(owner)
{
}

vds::async_task<const vds::server_log_new_object &>
vds::ichunk_manager::add(
  const const_data_buffer& data)
{
  return this->owner_->impl_->add(data);
}

vds::const_data_buffer vds::ichunk_manager::get(const guid & server_id, uint64_t index)
{
  return this->owner_->impl_->get(server_id, index);
}

void vds::ichunk_manager::set_next_index(uint64_t next_index)
{
  this->owner_->impl_->set_next_index(next_index);
}

//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager(
  const service_provider & sp,
  chunk_manager * owner)
: sp_(sp),
  owner_(owner),
  tmp_folder_(foldername(persistence::current_user(sp), ".vds"), "tmp"),
  last_tmp_file_index_(0),
  last_obj_file_index_(0),
  obj_size_(0)
{
  this->tmp_folder_.create();
}

vds::_chunk_manager::~_chunk_manager()
{
}

vds::async_task<const vds::server_log_new_object &>
vds::_chunk_manager::add(
  const const_data_buffer& data)
{
  return create_async_task(
    [data](const std::function<void (const void * data, size_t size)> & done, const error_handler & on_error){
      dataflow(
        deflate(),
        collect_data()
      )(
        done,
        on_error,
        data.data(),
        data.size()
      );
    })
  .then(
    [this,
    original_lenght = data.size(),
    original_hash = hash::signature(hash::sha256(), data)
    ](
      const std::function<void(const vds::server_log_new_object &)> & done,
      const error_handler & on_error,
      const void * deflated_data,
      size_t deflated_size) {
      
        this->tmp_folder_mutex_.lock();
        auto tmp_index = this->last_tmp_file_index_++;
        this->tmp_folder_mutex_.unlock();

        filename fn(this->tmp_folder_, std::to_string(tmp_index));
        file f(fn, file::create_new);
        f.write(deflated_data, deflated_size);
        f.close();

        this->obj_folder_mutex_.lock();
        auto index = this->last_obj_file_index_++;
        this->obj_size_ += deflated_size;
        this->obj_folder_mutex_.unlock();

        file::move(fn, this->cache_.get(this->sp_)
          .get_object_filename(this->storage_log_.get(this->sp_).current_server_id(), index));

        this->obj_folder_mutex_.lock();
        if (max_obj_size_ < this->obj_size_) {
          this->generate_chunk();
        }
        this->obj_folder_mutex_.unlock();

        auto result = server_log_new_object(
          index,
          original_lenght,
          std::move(original_hash),
          deflated_size,
          hash::signature(hash::sha256(), deflated_data, deflated_size));

        this->storage_log_.get(this->sp_).add_to_local_log(result.serialize().get());
        done(result);
      });
}

vds::const_data_buffer vds::_chunk_manager::get(
  const guid & server_id,
  uint64_t index)
{
  return inflate::inflate_buffer(file::read_all(this->cache_.get(this->sp_).get_object_filename(server_id, index)));
}

void vds::_chunk_manager::generate_chunk()
{
  throw new std::runtime_error("Not implemented");
}

void vds::_chunk_manager::set_next_index(uint64_t next_index)
{
  this->last_obj_file_index_ = next_index;
}

void vds::_chunk_manager::start()
{
  this->set_next_index(this->db_.get(this->sp_).last_object_index(this->storage_log_.get(this->sp_).current_server_id()));
}

void vds::_chunk_manager::stop()
{
}

