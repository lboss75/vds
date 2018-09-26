#ifndef __VDS_HTTP_HTTP_PIPELINE_H_
#define __VDS_HTTP_HTTP_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <queue>
#include "http_parser.h"
#include "http_serializer.h"
#include "http_response.h"

namespace vds {

  class http_pipeline : public http_parser_base<http_pipeline> {
  public:
    http_pipeline(
      const service_provider &sp,
      const std::function<std::future<http_message>(const http_message &message)> &message_callback,
      const std::shared_ptr<http_async_serializer> & output_stream)
      : http_parser_base<http_pipeline>([sp, message_callback, this](
        const http_message &message)->std::future<void> {
      auto pthis = this->shared_from_this();
      auto response = co_await message_callback(message);
      if (nullptr != response.body()) {
        this->send(sp, response);

      }
    }),
      output_stream_(output_stream) {
    }

    std::future<void> continue_read_data(const service_provider & sp) {
      auto continue_message = http_response::status_response(100, "Continue");
      co_await this->send(sp, continue_message);
    }

    void finish_message(const service_provider &sp) {
    }

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    std::mutex messages_queue_mutex_;
    std::queue<vds::http_message> messages_queue_;

    std::future<void> send(const vds::service_provider & sp, const vds::http_message & message) {
      vds_assert(message.body());

      std::unique_lock<std::mutex> lock(this->messages_queue_mutex_);
      if (!this->messages_queue_.empty()) {
        this->messages_queue_.emplace(message);
      }
      else {
        this->messages_queue_.emplace(message);
        lock.unlock();

        co_await this->continue_send(sp);
      }
    }

    std::future<void> continue_send(const vds::service_provider & sp) {
      for (;;) {
        co_await this->output_stream_->write_async(
          sp,
          this->messages_queue_.front());

        std::unique_lock<std::mutex> lock(this->messages_queue_mutex_);
        this->messages_queue_.pop();
        if (this->messages_queue_.empty()) {
          break;
        }
      }

    }
  };
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
