#ifndef __VDS_DATA_DEFLATE_P_H_
#define __VDS_DATA_DEFLATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <cstring>
#include <stdexcept>

#include "zlib.h"
#include "deflate.h"

namespace vds {

  class _deflate_handler
  {
  public:
    _deflate_handler(stream<uint8_t> & target, int compression_level)
    : target_(target)
    {
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != deflateInit(&this->strm_, compression_level)) {
        throw std::runtime_error("deflateInit failed");
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
        auto error = ::deflate(&this->strm_, Z_NO_FLUSH);
        
        if(Z_STREAM_ERROR == error){
          throw std::runtime_error("zlib error");
        }

        auto written = sizeof(output_data) - this->strm_.avail_out;
        this->target_.write(output_data, written);
      } while (0 == this->strm_.avail_out);
    }
    
    void final()
    {
      this->strm_.next_in = nullptr;
      this->strm_.avail_in = 0;

      uint8_t output_data[1024];
      do {
        this->strm_.next_out = (Bytef *)output_data;
        this->strm_.avail_out = sizeof(output_data);
        auto error = ::deflate(&this->strm_, Z_FINISH);
        
        if(Z_STREAM_ERROR == error){
          throw std::runtime_error("zlib error");
        }

        auto written = sizeof(output_data) - this->strm_.avail_out;
        this->target_.write(output_data, written);
      } while (0 == this->strm_.avail_out);
      
      deflateEnd(&this->strm_);
    }

  private:
    stream<uint8_t> & target_;
    z_stream strm_;
  };
}

#endif // __VDS_DATA_DEFLATE_P_H_
