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
#include "service_provider.h"

namespace vds {

	class _deflate_handler
	{
	public:
		_deflate_handler(
      const std::shared_ptr<stream_output_async<uint8_t>> & target,
        int compression_level)
			: target_(target)
		{
			memset(&this->strm_, 0, sizeof(z_stream));
			if (Z_OK != deflateInit(&this->strm_, compression_level)) {
				throw std::runtime_error("deflateInit failed");
			}
		}

		std::future<void> write_async(
      const service_provider & sp,
      const uint8_t * input_data,
      size_t input_size)
		{
			if (0 == input_size) {
        this->strm_.next_in = (Bytef *)input_data;
        this->strm_.avail_in = (uInt)input_size;

        uint8_t buffer[1024];
        do {
          this->strm_.next_out = (Bytef *)buffer;
          this->strm_.avail_out = sizeof(buffer);
          auto error = ::deflate(&this->strm_, Z_FINISH);

          if (Z_STREAM_ERROR == error) {
            throw std::runtime_error("deflate failed");
          }

          auto written = sizeof(buffer) - this->strm_.avail_out;
          co_await this->target_->write_async(sp, buffer, written);
        } while (0 == this->strm_.avail_out);

				deflateEnd(&this->strm_);
        co_await this->target_->write_async(sp, input_data, input_size);
				co_return;
			}

			this->strm_.next_in = (Bytef *)input_data;
			this->strm_.avail_in = (uInt)input_size;

			uint8_t buffer[1024];
			do {
				this->strm_.next_out = (Bytef *)buffer;
				this->strm_.avail_out = sizeof(buffer);
				auto error = ::deflate(&this->strm_, Z_NO_FLUSH);

				if (Z_STREAM_ERROR == error) {
					throw std::runtime_error("deflate failed");
				}

				auto written = sizeof(buffer) - this->strm_.avail_out;
        co_await this->target_->write_async(sp, buffer, written);
			} while (0 == this->strm_.avail_out);
		}

	private:
    std::shared_ptr<stream_output_async<uint8_t>> target_;
		z_stream strm_;
	};
}

#endif // __VDS_DATA_DEFLATE_P_H_
