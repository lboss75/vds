/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "deflate.h"
#include "deflate_p.h"

vds::deflate::deflate()
  : compression_level_(Z_DEFAULT_COMPRESSION)
{
}

vds::deflate::deflate(int compression_level)
  : compression_level_(compression_level)
{
}

vds::_deflate_handler * vds::deflate::create_handler() const
{
  return new _deflate_handler(this->compression_level_);
}

void vds::deflate::delete_handler(_deflate_handler * handler)
{
  delete handler;
}

bool vds::deflate::push_data(
  _deflate_handler * handler,
  const void * data,
  size_t size,
  const void *& to_push,
  size_t & to_push_len)
{
  return handler->push_data(data, size, to_push, to_push_len);
}

bool vds::deflate::data_processed(
  _deflate_handler * handler,
  const void *& to_push,
  size_t & to_push_len)
{
  return handler->processed(to_push, to_push_len);
}
///////////////////////////////////////////////////////////////
vds::_deflate_handler::_deflate_handler(
  int compression_level
) : eof_(false)
{
  memset(&this->strm_, 0, sizeof(z_stream));
  if (Z_OK != deflateInit(&this->strm_, compression_level)) {
    throw new std::runtime_error("deflateInit failed");
  }
}

bool vds::_deflate_handler::push_data(
  const void * data,
  size_t len,
  const void *& next_data,
  size_t & next_len)
{
  if (0 == len) {
    this->eof_ = true;
  }
  this->strm_.next_in = (Bytef *)data;
  this->strm_.avail_in = len;
  this->strm_.next_out = this->buffer_;
  this->strm_.avail_out = CHUNK_SIZE;
  auto error = ::deflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH);
  if (Z_OK != error) {
    throw new std::runtime_error("deflate failed");
  }

  if (CHUNK_SIZE != this->strm_.avail_out) {
    next_data = this->buffer_;
    next_len = CHUNK_SIZE - this->strm_.avail_out;
    return true;
  }
  else {
    return false;
  }
}

bool vds::_deflate_handler::processed(const void *& next_data, size_t & next_len)
{
  if (0 == this->strm_.avail_out) {
    this->strm_.next_out = this->buffer_;
    this->strm_.avail_out = CHUNK_SIZE;
    auto error = ::deflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH);
    if (0 > error) {
      throw new std::runtime_error("deflate failed");
    }

    next_data = this->buffer_;
    next_len = CHUNK_SIZE - this->strm_.avail_out;
    return true;
  }
  else {
    if (0 != this->strm_.avail_in) {
      throw new std::runtime_error("deflate failed");
    }

    if (!this->eof_) {
      return false;
    }
    else {
      deflateEnd(&this->strm_);

      next_data = nullptr;
      next_len = 0;
      return true;
    }
  }
}
