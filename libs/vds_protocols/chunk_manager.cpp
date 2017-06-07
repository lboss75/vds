/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"
#include "storage_log.h"
#include "hash.h"

vds::async_task<>
vds::ichunk_manager::add(
  const service_provider & sp,
  const guid & owner_principal,
  server_log_file_map & target,
  const filename & fn)
{
  return static_cast<_chunk_manager *>(this)->add(sp, owner_principal, target, fn);
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

vds::async_task<>
vds::_chunk_manager::add(
  const service_provider & sp,
  const guid & owner_principal,
  server_log_file_map & target,
  const filename & fn)
{
  return create_async_task(
    [this, fn, &target, owner_principal](
      const std::function<void (const service_provider & sp)> & done,
      const error_handler & on_error,
      const service_provider & sp){
        std::exception_ptr error;

        constexpr size_t BLOCK_SIZE = 5 * 1024 * 1024;
        auto file_size = file::length(fn);
        for (decltype(file_size) offset = 0; offset < file_size; offset += BLOCK_SIZE) {

          this->tmp_folder_mutex_.lock();
          auto tmp_index = this->last_tmp_file_index_++;
          this->tmp_folder_mutex_.unlock();

          filename tmp_file(this->tmp_folder_, std::to_string(tmp_index));

          size_t original_lenght;
          const_data_buffer original_hash;

          dataflow(
            file_range_read(fn, offset, BLOCK_SIZE),
            hash_filter(&original_lenght, &original_hash),
            file_write(tmp_file, file::file_mode::create_new)
          )(
            [this, tmp_file, &original_lenght, &original_hash, &target, owner_principal](const service_provider & sp) {

              this->obj_folder_mutex_.lock();
              auto index = this->last_obj_file_index_++;
              this->obj_folder_mutex_.unlock();

              file::move(tmp_file,
                sp.get<ilocal_cache>()->get_object_filename(
                  sp, sp.get<istorage_log>()->current_server_id(), index));

              auto result = server_log_new_object(
                index,
                original_lenght,
                original_hash,
                owner_principal);

              sp.get<istorage_log>()->add_to_local_log(sp, result.serialize());
              target.add(result);
          },
            [&error](const service_provider & sp, std::exception_ptr ex) {
              error = ex;
            },
            sp);
          if (error) {
            break;
          }
        };

        if (error) {
          on_error(sp, error);
        }
        else {
          done(sp);
        }
      });
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

