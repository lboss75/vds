/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "chunk_file.h"
#include "chunk_file_p.h"

static const char magic[] = "VDSC";

void vds::_chunk_file::read(
  const asymmetric_public_key & key,
  const filename & fn)
{
  file f(fn, file::open_read);

  char magic_data[sizeof(magic) - 1];
  if (sizeof(magic_data) != f.read(&magic_data, sizeof(magic_data))
    || 0 != memcmp(magic_data, magic, sizeof(magic_data))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  asymmetric_sign_verify v(hash::sha256(), key);

  //
  v.update(magic_data, sizeof(magic_data));

  //
  if (sizeof(this->source_id) != f.read(&this->source_id, sizeof(this->source_id))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  v.update(&this->source_id, sizeof(this->source_id));

  //
  if (sizeof(this->index) != f.read(&this->index, sizeof(this->index))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  v.update(&this->index, sizeof(this->index));

  //
  uint32_t size;
  if (sizeof(size) != f.read(&size, sizeof(size))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  v.update(&size, sizeof(size));

  std::vector<uint8_t> data(size);
  if (size != f.read(data.data(), size)) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  this->data.reset(data.data(), size);

  v.update(data.data(), size);

  //
  uint16_t len;
  if (sizeof(len) != f.read(&len, sizeof(len))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }
  data.resize(len);
  if (len != f.read(data.data(), len)) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }

  if (!v.verify(data_buffer(data.data(), len))) {
    throw new std::runtime_error("File corrupted " + fn.name());
  }
}

void vds::_chunk_file::write(
  const filename & fn,
  const asymmetric_private_key & key)
{
  asymmetric_sign sign(hash::sha256(), key);
  file f(fn, file::create_new);

  //
  sign.update(magic, sizeof(magic) - 1);
  f.write(magic, sizeof(magic) - 1);

  //
  sign.update(&this->source_id, sizeof(this->source_id));
  f.write(&this->source_id, sizeof(this->source_id));

  //
  sign.update(&this->index, sizeof(this->index));
  f.write(&this->index, sizeof(this->index));

  uint32_t size = (uint32_t)this->data.size();
  sign.update(&size, sizeof(size));
  f.write(&size, sizeof(size));

  //
  sign.update(this->data.data(), size);
  f.write(this->data.data(), size);

  sign.final();

  uint16_t len = (uint16_t)sign.signature().size();
  f.write(&len, sizeof(len));
  f.write(sign.signature().data(), len);
}

vds::_chunk_server::_chunk_server(
  const guid & source_id)
{
  this->last_chunk_file_.source_id.reset(source_id.data(), source_id.size());
  this->last_chunk_file_.index = 0;
}

uint64_t vds::_chunk_server::add_data(const void * data, size_t size)
{
  this->last_chunk_file_.index++;
  this->last_chunk_file_.data.reset(data, size);

  this->last_chunk_file_.write(
    filename(this->data_folder_, std::to_string(this->last_chunk_file_.index)),
    this->key_);

  return this->last_chunk_file_.index;
}


