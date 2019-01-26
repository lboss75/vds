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
    expected<void> create(
      const std::shared_ptr<stream_output_async<uint8_t>> & target,
      int compression_level) {

      this->target_ = target;
      memset(&this->strm_, 0, sizeof(z_stream));
      if (Z_OK != deflateInit(&this->strm_, compression_level)) {
        return make_unexpected<std::runtime_error>("deflateInit failed");
      }

      return expected<void>();
		}


		vds::async_task<expected<void>> write_async(      
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
            co_return make_unexpected<std::runtime_error>("deflate failed");
          }

          auto written = sizeof(buffer) - this->strm_.avail_out;
          CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(buffer, written));
        } while (0 == this->strm_.avail_out);

				deflateEnd(&this->strm_);
        CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(input_data, input_size));
				co_return expected<void>();
			}

			this->strm_.next_in = (Bytef *)input_data;
			this->strm_.avail_in = (uInt)input_size;

			uint8_t buffer[1024];
			do {
				this->strm_.next_out = (Bytef *)buffer;
				this->strm_.avail_out = sizeof(buffer);
				auto error = ::deflate(&this->strm_, Z_NO_FLUSH);

				if (Z_STREAM_ERROR == error) {
          co_return make_unexpected<std::runtime_error>("deflate failed");
				}

				auto written = sizeof(buffer) - this->strm_.avail_out;
        if (0 != written) {
          CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(buffer, written));
        }
			} while (0 == this->strm_.avail_out);

      co_return expected<void>();
		}

	private:
    std::shared_ptr<stream_output_async<uint8_t>> target_;
		z_stream strm_;
	};
}

#endif // __VDS_DATA_DEFLATE_P_H_
