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
    _inflate_handler()
    : state_(StateEnum::STATE_NORMAL)
    {
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != inflateInit(&this->strm_)) {
        throw std::runtime_error("inflateInit failed");
      }
    }

    void update_data(
      const void * input_data,
      size_t input_size,
      void * output_data,
      size_t output_size,
      size_t & readed,
      size_t & written)
    {
      if (this->state_ == StateEnum::STATE_NORMAL) {
        this->strm_.next_in = (Bytef *)input_data;
        this->strm_.avail_in = (uInt)input_size;
        this->state_ == StateEnum::STATE_PUSH;
      }

      this->strm_.next_out = (Bytef *)output_data;
      this->strm_.avail_out = output_size;
      auto result = ::inflate(&this->strm_, Z_NO_FLUSH);

      if (Z_STREAM_END == result) {
        this->state_ == StateEnum::STATE_EOF;
      }
      else if (Z_OK != result) {
        throw std::runtime_error("inflate failed");
      }

      written = output_size - this->strm_.avail_out;

      if (0 == this->strm_.avail_out) {
        readed = 0;
      }
      else {
        readed = input_size;
        if (StateEnum::STATE_EOF == this->state_) {
          inflateEnd(&this->strm_);
        }
        else {
          this->state_ = StateEnum::STATE_NORMAL;
        }
      }
    }

  private:
    z_stream strm_;
    enum class StateEnum
    {
      STATE_NORMAL,
      STATE_PUSH,
      STATE_EOF
    };
    StateEnum state_;
  };
}

#endif // __VDS_DATA_INFLATE_H_
