#ifndef __VDS_DATA_INFLATE_H_
#define __VDS_DATA_INFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "const_data_buffer.h"
#include "stream.h"

namespace vds {
  class _inflate_handler;
  class _inflate_async_handler;

  //Decompress stream
  class inflate : public stream<uint8_t>
  {
  public:
    inflate(stream<uint8_t> & target);
  };

  class inflate_async : public stream_async<uint8_t>
  {
  public:
	  inflate_async(stream_async<uint8_t> & target);
  };
}

#endif // __VDS_DATA_INFLATE_H_
