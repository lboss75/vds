#ifndef __VDS_DATA_INFLATE_P_H_
#define __VDS_DATA_INFLATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "zlib.h"

namespace vds {

  //Decompress stream
  class _inflate_handler
  {
  public:
    _inflate_handler(stream<uint8_t> & target)
    : target_(target)
    {
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != inflateInit(&this->strm_)) {
        throw std::runtime_error("inflateInit failed");
      }
    }

    void write(const uint8_t * input_data, size_t input_size)
    {
      this->strm_.next_in = (Bytef *)input_data;
      this->strm_.avail_in = (uInt)input_size;

      uint8_t output_data[1024];
      do {
        this->strm_.next_out = (Bytef *)output_data;
        this->strm_.avail_out = sizeof(output_data);
        auto result = ::inflate(&this->strm_, Z_NO_FLUSH);

        if (Z_STREAM_ERROR == result || Z_NEED_DICT == result || Z_DATA_ERROR == result || Z_MEM_ERROR == result) {
          throw std::runtime_error("inflate failed");
        }

        auto written = sizeof(output_data) - this->strm_.avail_out;
        this->target_.write(output_data, written);

      } while(0 == this->strm_.avail_out);
    }
    
    void final()
    {
      inflateEnd(&this->strm_);
    }

  private:
    stream<uint8_t> & target_;
    z_stream strm_;
  };
}

#endif // __VDS_DATA_INFLATE_H_
