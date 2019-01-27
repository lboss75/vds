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
      : target_(target)
    {
    }

    ~http_async_serializer()
    {
    }

    vds::async_task<vds::expected<void>> write_async(
      const http_message message)
    {
      auto pthis = this->shared_from_this();

      vds_assert(message.body());

      std::stringstream stream;
      for (auto & header : message.headers()) {
        stream << header << "\r\n";
      }
      stream << "\r\n";

      auto data = std::make_shared<std::string>(stream.str());
      CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(reinterpret_cast<const uint8_t *>(data->c_str()), data->length()));

      for (;;) {
        size_t readed;
        GET_EXPECTED_VALUE_ASYNC(readed, co_await message.body()->read_async(this->output_buffer_, sizeof(this->output_buffer_)));
        if (0 == readed) {
          CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(nullptr, 0));
          break;
        }

        CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(this->output_buffer_, readed));
      }

      co_return expected<void>();
    }

  private:
    std::shared_ptr<stream_output_async<uint8_t>> target_;
    uint8_t output_buffer_[1024];
  };

}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
