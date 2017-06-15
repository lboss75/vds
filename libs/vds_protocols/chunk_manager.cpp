/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"
#include "storage_log.h"
#include "hash.h"

vds::ichunk_manager::ichunk_manager()
{
}

vds::ichunk_manager::~ichunk_manager()
{
}

void vds::ichunk_manager::start(const service_provider & sp)
{
  static_cast<_chunk_manager *>(this)->start(sp);
}

void vds::ichunk_manager::stop(const service_provider & sp)
{
  static_cast<_chunk_manager *>(this)->stop(sp);
}

vds::async_task<> vds::ichunk_manager::add_object(
  const service_provider & sp,
  const guid & version_id,
  const filename & tmp_file)
{
  return static_cast<_chunk_manager *>(this)->add_object(
    sp,
    version_id,
    tmp_file);
}

/*
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
*/
//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager()
: chunk_storage_(MIN_HORCRUX)
{
}

vds::_chunk_manager::~_chunk_manager()
{
}

/*
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
        const std::shared_ptr<std::exception> & error;

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

              auto result = principal_log_new_object(
                index,
                original_lenght,
                original_hash,
                owner_principal);

              sp.get<istorage_log>()->add_to_local_log(sp, result.serialize());
              target.add(result);
          },
            [&error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
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
*/

void vds::_chunk_manager::start(const service_provider & sp)
{
  auto server_id = sp.get<istorage_log>()->current_server_id();
  this->last_chunk_ = sp.get<iserver_database>()->get_last_chunk(
    sp,
    server_id);
  
  this->tail_chunk_index_ = sp.get<iserver_database>()->get_tail_chunk(
    sp,
    server_id,
    this->tail_chunk_size_);
}

void vds::_chunk_manager::stop(const service_provider & sp)
{
}

vds::async_task<> vds::_chunk_manager::add_object(
  const service_provider & sp,
  const guid & version_id,
  const filename & tmp_file,
  const const_data_buffer & file_hash)
{
  return create_async_task([this, version_id, tmp_file, file_hash](
    const std::function<void (const service_provider & sp)> & done,
    const error_handler & on_error,
    const service_provider & sp){
    
    auto file_size = file::length(tmp_file);
    
    auto server_id = sp.get<istorage_log>()->current_server_id();
    principal_log_new_object result(server_id, version_id, file_size, file_hash, BLOCK_SIZE);
        
    for (decltype(file_size) offset = 0; offset < file_size; offset += BLOCK_SIZE) {
      if(BLOCK_SIZE < file_size - offset){
        if(!this->write_chunk(sp, result, tmp_file, offset, BLOCK_SIZE, on_error)){
          return;
        }
      }
      else {
        if(!this->write_tail(sp, result, tmp_file, offset, file_size - offset, on_error)){
          return;
        }
      }
    }
    
    sp.get<istorage_log>()->add_to_local_log(
      sp,
      server_id,
      sp.get<istorage_log>()->server_private_key(),
      result.serialize());
    
    done(sp);
  });
}

bool vds::_chunk_manager::write_chunk(
  const service_provider & sp,
  principal_log_new_object & result_record,
  const filename & fn,
  size_t offset,
  size_t size,
  const error_handler & on_error)
{
  this->chunk_mutex_.lock();
  auto index = this->last_chunk_++;
  this->chunk_mutex_.unlock();
  
  size_t original_length;
  const_data_buffer original_hash;

  bool result = true;
  std::vector<uint8_t> buffer;
  dataflow(
    file_range_read(fn, offset, size),
    hash_filter(&original_length, &original_hash),
    collect_data(buffer)
  )(
    [this, &result, &original_length, &original_hash, &buffer, index, &result_record, offset, size, on_error](const service_provider & sp){
      
      if (original_length != size) {
        on_error(sp, std::make_shared<std::runtime_error>("File is corrupt"));
        result = false;
        return;
      }

      result_record.add_chunk(index, original_hash);
      
      /*
      auto server_id = sp.get<istorage_log>()->current_server_id();
      sp.get<iserver_database>()->add_full_chunk(
        sp,
        version_id,
        offset,
        size,
        original_hash,
        server_id,
        index);
      */
      
      result = this->generate_horcruxes(
        sp,
        result_record.server_id(),
        index,
        buffer,
        on_error);
    },
    [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex){
      on_error(sp, ex);
      result = false;
    },
    sp);
  
  return result;
}

bool vds::_chunk_manager::write_tail(
  const service_provider & sp,
  const guid & version_id,
  const filename & fn,
  size_t offset,
  size_t size,
  const error_handler & on_error)
{
  auto server_id = sp.get<istorage_log>()->current_server_id();
  bool result = true;
  while(0 < size) {
    std::lock_guard<std::mutex> lock(this->tail_chunk_mutex_);

    auto index = this->tail_chunk_index_;

    auto to_write = BLOCK_SIZE - this->tail_chunk_size_;
    if (to_write <= size) {
      size_t original_length;
      const_data_buffer original_hash;
      std::vector<uint8_t> tail_buffer;
      dataflow(
        file_range_read(fn, offset, to_write),
        hash_filter(&original_length, &original_hash),
        collect_data(tail_buffer))(
          [this, sp, server_id, version_id, index, offset, to_write, &original_hash, &tail_buffer](const service_provider & sp) {
            sp.get<iserver_database>()->add_to_tail_chunk(
              sp,
              version_id,
              offset,
              original_hash,
              server_id,
              index,
              this->tail_chunk_size_,
              tail_buffer);
          },
          [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            on_error(sp, ex);
            result = false;
          },
          sp);
      if (!result) {
        break;
      }

      offset += to_write;
      size -= to_write;
      this->tail_chunk_size_ += to_write;

      if (BLOCK_SIZE == this->tail_chunk_size_) {
        this->chunk_mutex_.lock();
        this->tail_chunk_index_ = this->last_chunk_++;
        this->tail_chunk_size_ = 0;
        this->chunk_mutex_.unlock();

        sp.get<iserver_database>()->start_tail_chunk(
          sp,
          server_id,
          this->tail_chunk_index_);

        result = this->generate_tail_horcruxes(
          sp,
          server_id,
          index,
          on_error);

        if (!result) {
          break;
        }
      }
    }
    else {
      size_t original_length;
      const_data_buffer original_hash;
      std::vector<uint8_t> tail_buffer;
      dataflow(
        file_range_read(fn, offset, size),
        hash_filter(&original_length, &original_hash),
        collect_data(tail_buffer))(
          [this, sp, server_id, version_id, index, offset, size, &original_hash, &tail_buffer](const service_provider & sp) {
            sp.get<iserver_database>()->add_to_tail_chunk(
              sp,
              version_id,
              offset,
              original_hash,
              server_id,
              index,
              this->tail_chunk_size_,
              tail_buffer);
          },
          [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
            on_error(sp, ex);
            result = false;
          },
          sp);
        
      if (result) {
        this->tail_chunk_size_ += size;
      }
      
      break;
    }
  }

  return result;
}

bool vds::_chunk_manager::generate_tail_horcruxes(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index,
  const error_handler & on_error)
{
  auto data = sp.get<iserver_database>()->get_tail_data(sp, server_id, chunk_index);

  size_t original_length;
  const_data_buffer original_hash;

  bool result = true;
  std::vector<uint8_t> buffer;
  dataflow(
    dataflow_arguments<uint8_t>(data.data(), data.size()),  
    hash_filter(&original_length, &original_hash),
    collect_data(buffer)
  )(
    [this, &result, &original_length, &original_hash, &buffer, server_id, chunk_index, on_error](const service_provider & sp) {

      sp.get<iserver_database>()->final_tail_chunk(
        sp,
        original_length,
        original_hash,
        server_id,
        chunk_index);

      result = this->generate_horcruxes(
        sp,
        server_id,
        chunk_index,
        buffer,
        on_error);
    },
    [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
      on_error(sp, ex);
      result = false;
    },
    sp);
  return result;
}

bool vds::_chunk_manager::generate_horcruxes(
  const service_provider & sp,
  const guid & server_id,
  size_t chunk_index,
  const std::vector<uint8_t> & buffer,
  const error_handler & on_error)
{
  bool result = true;

  for (uint16_t replica = 0; replica < GENERATE_HORCRUX; ++replica) {

    auto replica_data = this->chunk_storage_.generate_replica(replica, buffer.data(), buffer.size());
    size_t replica_length;
    const_data_buffer replica_hash;
    std::vector<uint8_t> buffer;
    dataflow(
      dataflow_arguments<uint8_t>(replica_data.data(), replica_data.size()),
      hash_filter(&replica_length, &replica_hash),
      collect_data(buffer))(
      [server_id, chunk_index, replica, &replica_length, &replica_hash, &buffer](const service_provider & sp) {
          sp.get<iserver_database>()->add_chunk_replica(
            sp,
            server_id,
            chunk_index,
            replica,
            replica_length,
            replica_hash);

          sp.get<iserver_database>()->add_chunk_store(
            sp,
            server_id,
            chunk_index,
            replica,
            sp.get<istorage_log>()->current_server_id(),
            buffer);
        },
        [&result, on_error](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
          on_error(sp, ex);
          result = false;
        },
        sp);
    if (!result) {
      break;
    }
  }

  return result;
}
