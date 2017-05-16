/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "inflate.h"
#include "inflate_p.h"

vds::inflate::inflate()
{
}

vds::_inflate_handler * vds::inflate::create_handler() const
{
  return new _inflate_handler();
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

vds::const_data_buffer vds::inflate::inflate_buffer(const const_data_buffer & data)
{
  std::vector<uint8_t> result;
  
  _inflate_handler handler;
  
  const void * to_push;
  size_t to_push_len;
  if(handler.push_data(data.data(), data.size(), to_push, to_push_len)){
    while(0 != to_push_len){
      result.insert(
        result.end(),
        reinterpret_cast<const uint8_t *>(to_push),
        reinterpret_cast<const uint8_t *>(to_push) + to_push_len);
      
      if (!handler.processed(to_push, to_push_len)) {
        if(handler.push_data(nullptr, 0, to_push, to_push_len)){
          while(0 != to_push_len){
            result.insert(
              result.end(),
              reinterpret_cast<const uint8_t *>(to_push),
              reinterpret_cast<const uint8_t *>(to_push) + to_push_len);
            
            if (!handler.processed(to_push, to_push_len)) {
              break;
            }
          }
        }
        
        break;
      }
    }
  }
  
  return const_data_buffer(result.data(), result.size());
}

///////////////////////////////////////////////
vds::_inflate_handler::_inflate_handler()
: eof_(false)
{
  memset(&this->strm_, 0, sizeof(z_stream));
  if (Z_OK != inflateInit(&this->strm_)) {
    throw std::runtime_error("inflateInit failed");
  }
}

bool vds::_inflate_handler::push_data(const void * data, size_t len, const void *& next_data, size_t & next_len)
{
  if (0 == len) {
    this->eof_ = true;
  }
  this->strm_.next_in = (Bytef *)data;
  this->strm_.avail_in = (uInt)len;
  this->strm_.next_out = this->buffer_;
  this->strm_.avail_out = CHUNK_SIZE;
  auto result = ::inflate(&this->strm_, Z_NO_FLUSH);
  if (Z_STREAM_END == result) {
    this->eof_ = true;
  }
  else if (Z_OK != result) {
    throw std::runtime_error("inflate failed");
  }

  next_data = this->buffer_;
  next_len = CHUNK_SIZE - this->strm_.avail_out;
  return true;
}

bool vds::_inflate_handler::processed(const void *& next_data, size_t & next_len)
{
  if (0 == this->strm_.avail_out) {
    this->strm_.next_out = this->buffer_;
    this->strm_.avail_out = CHUNK_SIZE;
 
    auto result = ::inflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH);
    if (Z_STREAM_END == result) {
      this->eof_ = true;
    }
    else if (Z_OK != result) {
      if (Z_BUF_ERROR == result) {
        return false;
      }
      throw std::runtime_error("inflate failed");
    }

    next_data = this->buffer_;
    next_len = CHUNK_SIZE - this->strm_.avail_out;
    return true;
  }
  else {
    if (0 != this->strm_.avail_in) {
      throw std::runtime_error("inflate failed");
    }

    if (!this->eof_) {
      return false;
    }
    else {
      inflateEnd(&this->strm_);

      next_data = nullptr;
      next_len = 0;
      return true;
    }
  }
}


