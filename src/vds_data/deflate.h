#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "zlib.h"

namespace vds {
  
  //Compress data
  class deflate
  {
  public:
    deflate(int compression_level = Z_DEFAULT_COMPRESSION)
    : compression_level_(compression_level)
    {
    }
    
    template <typename context_type>
    class handler : public sequence_step<context_type, void (const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void (const void *, size_t)>;
    public:
      handler(
	const context_type & context,
	const deflate & args
      ) : base_class(context), eof_(false)
      {
	memset(&this->strm_, 0, sizeof(z_stream));
	if (Z_OK != deflateInit(&this->strm_, args.compression_level_)) {
	  throw new std::runtime_error("deflateInit failed");
	}
      }
      
      void operator()(const void * data, size_t len){
	if (0 == len) {
	  this->eof_ = true;
	}
	this->strm_.next_in = data;
	this->strm_.avail_in = len;
	this->strm_.next_out = this->buffer_;
	this->strm_.avail_out = CHUNK_SIZE;
	if (Z_STREAM_ERROR != ::deflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH)) {
	  throw new std::runtime_error("deflate failed");
	}
	
	this->next(
	  this->buffer_,
	  CHUNK_SIZE - this->strm_.avail_out
	);
      }
      
      void processed()
      {
	if(0 == this->strm_.avail_out){
	  if (Z_STREAM_ERROR != ::deflate(&this->strm_, this->eof_ ? Z_FINISH : Z_NO_FLUSH)) {
	    throw new std::runtime_error("deflate failed");
	  }
	  
	  this->next(
	    this->buffer_,
	    CHUNK_SIZE - this->strm_.avail_out
	  );
	}
	else {
	  if(0 != this->strm_.avail_in){
	    throw new std::runtime_error("deflate failed");
	  }
	  
	  if(!this->eof_){
	    this->prev();
	  }
	  else {
	    deflateEnd(&this->strm_);
	  }
	}
      }
      
    private:
      static constexpr size_t CHUNK_SIZE;
      unsigned char buffer_[CHUNK_SIZE];
      z_stream strm_;
      bool eof_;
    };
  private:
    int compression_level_;
  };
}

#endif // __VDS_DATA_DEFLATE_H_
