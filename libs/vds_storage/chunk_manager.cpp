/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"

vds::chunk_manager::chunk_manager(
  const service_provider & sp,
  const guid & server_id,
  local_cache & cache)
  : impl_(new _chunk_manager(sp, server_id, cache, this))
{
}

vds::chunk_manager::~chunk_manager()
{
  delete this->impl_;
}

void vds::chunk_manager::add(
  const std::function<void (chunk_manager::file_map) > & done,
  const error_handler & on_error,
  const filename & fn)
{
  this->impl_->add(done, on_error, fn);
}

void vds::chunk_manager::add(
  const std::function<void (chunk_manager::object_index) > & done,
  const error_handler & on_error,
  const data_buffer& data)
{
  this->impl_->add(done, on_error, data);
}

void vds::chunk_manager::set_next_index(uint64_t next_index)
{
  this->impl_->set_next_index(next_index);
}

//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager(
  const service_provider & sp,
  const guid & server_id,
  local_cache & cache,
  chunk_manager * owner)
: owner_(owner),
  server_id_(server_id),
  cache_(cache),
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

void vds::_chunk_manager::add(
  const std::function<void (vds::chunk_manager::file_map) > & done,
  const error_handler & on_error,
  const filename & fn)
{
  dataflow(
    read_file(fn, (size_t)5 * 1024 * 1024 * 1024),
    task_step([this](
      const std::function<void(void) > & done,
      const error_handler & on_error,
      const std::function<void(void)> & prev,
      const void * data, size_t size){
      if(0 == size){
        done();
        return;
      }
      
      this->add(
        [prev](vds::chunk_manager::object_index){
          prev();
        },
        on_error,
        data_buffer(data, size));
    }))
  (
    [](){},
    on_error);
}

void vds::_chunk_manager::add(
  const std::function<void (vds::chunk_manager::object_index) > & done,
  const error_handler & on_error,
  const data_buffer& data)
{
  auto steps = async_task(
    [](const std::function<void (const void * data, size_t size)> & done,
       const error_handler & on_error,
       const void * data, size_t size){
      dataflow(
        deflate(),
        collect_data()
      )(
        done,
        on_error,
        data,
        size
      );
    })
  ->then(
    [this](
      const std::function<void (vds::chunk_manager::object_index)> & done,
      const error_handler & on_error,
      const void * deflated_data, size_t deflated_size){
      
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
      
      file::move(fn, this->cache_.get_object_filename(this->server_id_, index));
      
      this->obj_folder_mutex_.lock();
      if(max_obj_size_ < this->obj_size_){
        this->generate_chunk();
      }
      this->obj_folder_mutex_.unlock();
      
      done(chunk_manager::object_index {index});
    });
  
  steps->invoke(
    [steps, done](chunk_manager::object_index index){
      done(index);
    },
    [steps, on_error](std::exception_ptr ex){
      on_error(ex);
    },
    data.data(),
    data.size());
}

void vds::_chunk_manager::generate_chunk()
{
  throw new std::runtime_error("Not implemented");
}

void vds::_chunk_manager::set_next_index(uint64_t next_index)
{
  this->last_obj_file_index_ = next_index;
}
