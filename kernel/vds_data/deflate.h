#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stream.h"

namespace vds {
  class _deflate_handler;
  class _deflate_async_handler;
  class const_data_buffer;

  //Compress data
  class deflate : public stream<uint8_t>
  {
  public:
    deflate(stream<uint8_t> & target);
    deflate(stream<uint8_t> & target, int compression_level);

    static const_data_buffer compress(
      const uint8_t * data,
      size_t len);
    
    static const_data_buffer compress(
      const const_data_buffer & data);
  };

  class deflate_async : public stream_async<uint8_t>
  {
  public:
	  deflate_async(stream_async<uint8_t> & target);
	  deflate_async(stream_async<uint8_t> & target, int compression_level);
  };
}

#endif // __VDS_DATA_DEFLATE_H_
