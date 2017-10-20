#ifndef __VDS_DATA_INFLATE_P_H_
#define __VDS_DATA_INFLATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "zlib.h"

namespace vds {

  //Decompress stream
  class _inflate_handler : public std::enable_shared_from_this<_inflate_handler>
  {
  public:
    _inflate_handler(stream_asynñ<uint8_t> & target)
    : target_(target)
    {
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != inflateInit(&this->strm_)) {
        throw std::runtime_error("inflateInit failed");
      }
    }

    async_task<> write_async(const uint8_t * input_data, size_t input_size)
    {
      if(0 == input_size){
        inflateEnd(&this->strm_);
        return this->target_.write_async(input_data, input_size);
      }
      
      this->strm_.next_in = (Bytef *)input_data;
      this->strm_.avail_in = (uInt)input_size;

      return [pthis = this->shared_from_this()](const async_result<> & result){
        pthis->continue_write(result);
      };
    }

  private:
    stream_asynñ<uint8_t> & target_;
    z_stream strm_;
    uint8_t buffer_[1024];
    
    void continue_write(const async_result<> & result)
    {
      if(0 != this->strm_.avail_out){
        result();
      }
      else {
        this->strm_.next_out = (Bytef *)this->buffer_;
        this->strm_.avail_out = sizeof(this->buffer_);
        auto error = ::inflate(&this->strm_, Z_NO_FLUSH);

        if (Z_STREAM_ERROR == error || Z_NEED_DICT == error || Z_DATA_ERROR == error || Z_MEM_ERROR == error) {
          result.error(std::make_shared<std::runtime_error>("inflate failed"));
          return;
        }

        auto written = sizeof(buffer_) - this->strm_.avail_out;
        this->target_.write_async(buffer_, written)
        .wait(
          [pthis = this->shared_from_this(), result](){
            pthis->continue_write(result);
          },
          [result](const std::shared_ptr<std::exception> & ex){
           result.error(ex); 
          });
      }
    }
    
  };
}

#endif // __VDS_DATA_INFLATE_H_
