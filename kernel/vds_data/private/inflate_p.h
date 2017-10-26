#ifndef __VDS_DATA_INFLATE_P_H_
#define __VDS_DATA_INFLATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "zlib.h"
#include "service_provider.h"

namespace vds {

  //Decompress stream
  class _inflate_handler : public std::enable_shared_from_this<_inflate_handler>
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

    void write(const service_provider & sp, const uint8_t * input_data, size_t input_size)
    {
      if(0 == input_size){
        inflateEnd(&this->strm_);
        this->target_.write(sp, input_data, input_size);
        return;
      }
      
      this->strm_.next_in = (Bytef *)input_data;
      this->strm_.avail_in = (uInt)input_size;

      uint8_t buffer[1024];
      do{
        this->strm_.next_out = (Bytef *)buffer;
        this->strm_.avail_out = sizeof(buffer);
        auto error = ::inflate(&this->strm_, Z_NO_FLUSH);

        if (Z_STREAM_ERROR == error || Z_NEED_DICT == error || Z_DATA_ERROR == error || Z_MEM_ERROR == error) {
          throw std::runtime_error("inflate failed");
        }

        auto written = sizeof(buffer) - this->strm_.avail_out;
        this->target_.write(sp, buffer, written);
      } while(0 == this->strm_.avail_out);
    }

  private:
    stream<uint8_t> & target_;
    z_stream strm_;
  };
  
  //Decompress stream
  class _inflate_async_handler : public std::enable_shared_from_this<_inflate_async_handler>
  {
  public:
    _inflate_async_handler(stream_async<uint8_t> & target)
    : target_(target)
    {
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != inflateInit(&this->strm_)) {
        throw std::runtime_error("inflateInit failed");
      }
    }

    async_task<> write_async(
      const service_provider & sp,
      const uint8_t * input_data,
      size_t input_size)
    {
      if(0 == input_size){
        inflateEnd(&this->strm_);
        return this->target_.write_async(sp, input_data, input_size);
      }
      
      this->strm_.next_in = (Bytef *)input_data;
      this->strm_.avail_in = (uInt)input_size;

      return [pthis = this->shared_from_this(), sp](const async_result<> & result){
        pthis->continue_write(sp, result);
      };
    }

  private:
    stream_async<uint8_t> & target_;
    z_stream strm_;
    uint8_t buffer_[1024];
    
    void continue_write(const service_provider & sp, const async_result<> & result)
    {
      if(0 != this->strm_.avail_out){
        result.done();
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
        this->target_.write_async(sp, buffer_, written)
        .execute(
          [pthis = this->shared_from_this(), result, sp](const std::shared_ptr<std::exception> & ex){
            if(ex){
              result.error(ex);
            }
            else {
              pthis->continue_write(sp, result);
            }
          });
      }
    }
    
  };
}

#endif // __VDS_DATA_INFLATE_H_
