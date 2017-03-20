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

uint64_t vds::chunk_manager::start_stream()
{
  return this->start_stream();
}

void vds::chunk_manager::add_stream(uint64_t id, const void * data, size_t len)
{
  this->impl_->add_stream(id, data, len);
}

void vds::chunk_manager::finish_stream(uint64_t id)
{
  this->impl_->finish_stream(id);
}

//////////////////////////////////////////////////////////////////////
uint64_t vds::_chunk_manager::start_stream()
{
  std::lock_guard<std::mutex> lock(this->file_mutex_);
  
  uint8_t marker = (uint8_t)cbt_start_stream;
  this->output_file_.write(&marker, sizeof(marker));

  uint64_t id_marker = (uint64_t)this->last_index_++;
  this->output_file_.write(&id_marker, sizeof(id_marker));

  return id_marker;
}

void vds::_chunk_manager::add_stream(uint64_t id, const void * data, size_t len)
{
  std::lock_guard<std::mutex> lock(this->file_mutex_);

  uint8_t marker = (uint8_t)cbt_add_stream;
  this->output_file_.write(&marker, sizeof(marker));

  uint64_t id_marker = (uint64_t)id;
  this->output_file_.write(&id_marker, sizeof(id_marker));

  uint64_t len_marker = (uint64_t)len;
  this->output_file_.write(&len_marker, sizeof(len_marker));

  this->output_file_.write(data, len);

  if (output_file_max_size < this->output_file_.length()) {
    this->generate_chunk();
  }
}

void vds::_chunk_manager::finish_stream(uint64_t id)
{
  std::lock_guard<std::mutex> lock(this->file_mutex_);

  uint8_t marker = (uint8_t)cbt_finish_stream;
  this->output_file_.write(&marker, sizeof(marker));

  uint64_t id_marker = (uint64_t)id;
  this->output_file_.write(&id_marker, sizeof(id_marker));
}

void vds::_chunk_manager::generate_chunk()
{
  throw new std::runtime_error("Not implemented");
}
