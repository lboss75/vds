/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_manager.h"
#include "chunk_manager_p.h"

vds::chunk_manager::chunk_manager(const service_provider & sp)
  : impl_(new _chunk_manager(sp, this))
{
}

vds::chunk_manager::~chunk_manager()
{
  delete this->impl_;
}

uint64_t vds::chunk_manager::add(const data_buffer& data)
{
  return this->impl_->add(data);
}

void vds::chunk_manager::add(const filename& fn, std::list< uint64_t >& parts)
{
  this->impl_->add(fn, parts);
}
//////////////////////////////////////////////////////////////////////
vds::_chunk_manager::_chunk_manager(
  const service_provider & sp,
  chunk_manager * owner)
: owner_(owner),
  tmp_folder_(foldername(persistence::current_user(sp), ".vds"), "tmp"),
  last_tmp_file_index_(0),
  obj_folder_(foldername(persistence::current_user(sp), ".vds"), "objects"),
  last_obj_file_index_(0),
  obj_size_(0)
{
  this->tmp_folder_.create();
  this->obj_folder_.create();
}

vds::_chunk_manager::~_chunk_manager()
{
}

void vds::_chunk_manager::add(const filename& fn, std::list<uint64_t>& parts)
{
  uint8_t buffer[(size_t)5 * 1024 * 1024 * 1024];
  
  file f(fn, file::open_read);
  for(;;){
    auto readed = f.read(buffer, sizeof(buffer));
    if(0 == readed){
      break;
    }
    
    parts.push_back(this->add(data_buffer(buffer, readed)));
  }
}

uint64_t vds::_chunk_manager::add(const data_buffer& data)
{
  this->tmp_folder_mutex_.lock();
  auto tmp_index = this->last_tmp_file_index_++;
  this->tmp_folder_mutex_.unlock();
  
  filename fn(this->tmp_folder_, std::to_string(tmp_index));
  sequence(
    deflate(),
    write_file(fn, file::create_new)       
  )
  (
   []() {},
   [](std::exception * ex) { throw ex; },
   data.data(),
   data.size());
  
  auto flen = file::length(fn);
  
  this->obj_folder_mutex_.lock();
  auto index = this->last_obj_file_index_++;
  this->obj_size_ += flen;
  this->obj_folder_mutex_.unlock();
  
  filename target_file(this->obj_folder_, std::to_string(index));
  file::move(fn, target_file);
  
  this->obj_folder_mutex_.lock();
  if(max_obj_size_ < this->obj_size_){
    this->generate_chunk();
  }
  this->obj_folder_mutex_.unlock();
  
  return index;
}

void vds::_chunk_manager::generate_chunk()
{
  throw new std::runtime_error("Not implemented");
}
