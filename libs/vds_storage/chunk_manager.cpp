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
  const asymmetric_private_key & private_key,
  local_cache & cache)
  : impl_(new _chunk_manager(sp, server_id, private_key, cache, this))
{
}

vds::chunk_manager::~chunk_manager()
{
  delete this->impl_;
}

vds::async_task<const vds::chunk_manager::file_map &>
vds::chunk_manager::add(
  const filename & fn)
{
  return this->impl_->add(fn);
}

vds::async_task<const vds::chunk_manager::object_index &>
vds::chunk_manager::add(
  const data_buffer& data)
{
  return this->impl_->add(data);
}

void vds::chunk_manager::set_next_index(uint64_t next_index)
{
  this->impl_->set_next_index(next_index);
}

//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager(
  const service_provider & sp,
  const guid & server_id,
  const asymmetric_private_key & private_key,
  local_cache & cache,
  chunk_manager * owner)
: owner_(owner),
  private_key_(private_key),
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

vds::async_task<const vds::chunk_manager::file_map &>
vds::_chunk_manager::add(
  const filename & fn)
{
  auto result = std::make_shared<chunk_manager::file_map>();

  return create_async_task(
    [this, fn, result](const std::function<void(const chunk_manager::file_map &)> & done, const error_handler & on_error) {
    dataflow(
      read_file(fn, (size_t)5 * 1024 * 1024 * 1024),
      task_step([this, result](
        const std::function<void(void) > & done,
        const error_handler & on_error,
        const std::function<void(void)> & prev,
        const void * data, size_t size) {
      if (0 == size) {
        done();
        return;
      }

      this->add(data_buffer(data, size)).wait(
        [prev, result](chunk_manager::object_index index) {
        result->add(index);
        prev();
      },
        on_error);
    }))
        (
          [done, result]() { done(*result); },
          on_error);
  });
}

vds::async_task<const vds::chunk_manager::object_index &>
vds::_chunk_manager::add(
  const data_buffer& data)
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
      const std::function<void(const vds::chunk_manager::object_index &)> & done,
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

        file::move(fn, this->cache_.get_object_filename(this->server_id_, index));

        this->obj_folder_mutex_.lock();
        if (max_obj_size_ < this->obj_size_) {
          this->generate_chunk();
        }
        this->obj_folder_mutex_.unlock();

        done(chunk_manager::object_index(
          index,
          original_lenght,
          std::move(original_hash),
          deflated_size,
          hash::signature(hash::sha256(), deflated_data, deflated_size)));
      });
}

void vds::_chunk_manager::generate_chunk()
{
  throw new std::runtime_error("Not implemented");
}

void vds::_chunk_manager::set_next_index(uint64_t next_index)
{
  this->last_obj_file_index_ = next_index;
}

void vds::chunk_manager::file_map::add(const object_index & item)
{
}

vds::chunk_manager::object_index::object_index(
  uint64_t index,
  uint32_t original_lenght,
  const vds::data_buffer & original_hash,
  uint32_t target_lenght,
  const vds::data_buffer& target_hash)
: index_(index),
  original_lenght_(original_lenght),
  original_hash_(original_hash),
  target_lenght_(target_lenght),
  target_hash_(target_hash)
{
}
