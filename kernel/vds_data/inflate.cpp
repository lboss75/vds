/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "inflate_p.h"

vds::inflate::inflate()
  : compression_level_(Z_DEFAULT_COMPRESSION)
{
}

vds::inflate::inflate(int compression_level)
  : compression_level_(compression_level)
{
}

vds::_inflate_handler * vds::inflate::create_handler() const
{
  return new vds::_inflate_handler(this->compression_level_);
}

void vds::inflate::delete_handler(_inflate_handler * handler)
{
  delete handler;
}

bool vds::inflate::push_data(_inflate_handler * handler, const void * data, size_t size, const void *& to_push, size_t & to_push_len)
{
  return handler->push_data(data, size, to_push, to_push_len);
}

bool vds::inflate::data_processed(_inflate_handler * handler, const void *& to_push, size_t & to_push_len)
{
  return handler->processed(to_push, to_push_len);
}
///////////////////////////////////////////////
vds::_inflate_handler::_inflate_handler(int compression_level)
: eof_(false)
{
  memset(&this->strm_, 0, sizeof(z_stream));
  if (Z_OK != inflateInit(&this->strm_, args.compression_level_)) {
    throw new std::runtime_error("inflateInit failed");
  }
}

bool vds::_inflate_handler::push_data(const void * data, size_t len, const void *& next_data, size_t & next_len)
{
  if (0 == len) {
    this->eof_ = true;
  }
  this->strm_.next_in = (Bytef *)data;
  this->strm_.avail_in = len;
  this->strm_.next_out = this->buffer_;
  this->strm_.avail_out = CHUNK_SIZE;
  auto result = ::inflate(&this->strm_, Z_NO_FLUSH);
  if (Z_STREAM_END == result) {
    this->eof_ = true;
  }
  else if (Z_STREAM_ERROR != result) {
    throw new std::runtime_error("inflate failed");
  }

  next_data = this->buffer_;
  next_len = CHUNK_SIZE - this->strm_.avail_out;
  return true;
}

bool vds::_inflate_handler::processed(const void *& next_data, size_t & next_len)
{
  if (0 == this->strm_.avail_out) {

    auto result = ::inflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH);
    if (Z_STREAM_END == result) {
      this->eof_ = true;
    }
    else if (Z_STREAM_ERROR != result) {
      throw new std::runtime_error("inflate failed");
    }

    next_data = this->buffer_;
    next_len = CHUNK_SIZE - this->strm_.avail_out;
    return true;
  }
  else {
    if (0 != this->strm_.avail_in) {
      throw new std::runtime_error("inflate failed");
    }

    if (!this->eof_) {
      return false;
    }
    else {
      inflateEnd(&this->strm_);

      next_data = this->buffer_;
      next_len = CHUNK_SIZE - this->strm_.avail_out;
      return true;
    }
  }
}


