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
    ~inflate();

    void write(const uint8_t * data, size_t len) override;


  private:
    _inflate_handler * const impl_;
  };

  class inflate_async : public stream_asynñ<uint8_t>
  {
  public:
	  inflate_async(stream_asynñ<uint8_t> & target);
	  ~inflate_async();

	  async_task<> write_async(const uint8_t * data, size_t len) override;


  private:
	  _inflate_handler * const impl_;
  };
}

#endif // __VDS_DATA_INFLATE_H_
