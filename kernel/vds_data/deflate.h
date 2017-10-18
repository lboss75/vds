#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stream.h"

namespace vds {
  class _deflate_handler;

  //Compress data
  class deflate : public stream<uint8_t>
  {
  public:
    deflate(stream<uint8_t> & target);
    deflate(stream<uint8_t> & target, int compression_level);
    ~deflate();

    async_task<> write_async(const uint8_t * data, size_t len) override;

  private:
      _deflate_handler * const impl_;
  };
}

#endif // __VDS_DATA_DEFLATE_H_
