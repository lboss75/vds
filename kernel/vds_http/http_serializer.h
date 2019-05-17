#ifndef __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
#define __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_buffer.h"
#include "http_message.h"
#include "http_request.h"
#include "json_object.h"
#include "vds_debug.h"

namespace vds {
  class http_async_serializer : public std::enable_shared_from_this<http_async_serializer>
  {
  public:
    http_async_serializer(
      const std::shared_ptr<stream_output_async<uint8_t>> & target)
      : target_(target), write_body_(false)
    {
    }

    ~http_async_serializer()
    {
    }

    vds::async_task<vds::expected<std::shared_ptr<stream_output_async<uint8_t>>>> start_message(
		const std::list<std::string> & headers)
    {
		vds_assert(!this->write_body_);

      std::stringstream stream;
      for (auto & header : headers) {
        stream << header << "\r\n";
      }
      stream << "\r\n";

      auto data = stream.str();
      CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(reinterpret_cast<const uint8_t *>(data.c_str()), data.length()));

	  this->write_body_ = true;
	  
      co_return std::make_shared<out_stream>(this->shared_from_this());
    }

  private:
    std::shared_ptr<stream_output_async<uint8_t>> target_;
	bool write_body_;

	class out_stream : public stream_output_async<uint8_t>
	{
	public:
		out_stream(std::shared_ptr<http_async_serializer> target)
		: target_(target) {
		}

		async_task<expected<void>> write_async(
			const uint8_t *data,
			size_t len) override {

			if (0 != len) {
				co_await this->target_->target_->write_async(data, len);
			}
			else {
				this->target_->write_body_ = false;
			}

			co_return expected<void>();
		}

	private:
		std::shared_ptr<http_async_serializer> target_;
	};

  };

}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
