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

  class _http_pipeline : public _http_parser_base<_http_pipeline> {
  public:
    _http_pipeline(
        const service_provider &sp,
        const std::function<async_task<http_message>(const http_message &message)> &message_callback,
        const std::shared_ptr<http_async_serializer> & output_stream)
        : _http_parser_base<_http_pipeline>(sp, [sp, message_callback, this](
          const http_message &message)->async_task<> {
      auto pthis = this->shared_from_this();
      return message_callback(message).then([sp, pthis](const http_message &response) {
        auto this_ = static_cast<_http_pipeline *>(pthis.get());
        this_->send(sp, response);
      });
    }),
      output_stream_(output_stream) {
      }

    void continue_read_data(const service_provider &sp) {
      auto continue_message = http_response::status_response(sp, 100, "Continue");
      this->send(sp, continue_message);
    }

    void finish_message(const service_provider &sp) {
    }

  private:
    std::shared_ptr<http_async_serializer> output_stream_;

    std::mutex messages_queue_mutex_;
    std::queue<vds::http_message> messages_queue_;
    bool send_continue_;

    void send(const vds::service_provider & sp, const vds::http_message & message) {
      vds_assert(message.body());

      std::unique_lock<std::mutex> lock(this->messages_queue_mutex_);
      if(!this->messages_queue_.empty()) {
        this->messages_queue_.emplace(message);
      }
      else {
        this->messages_queue_.emplace(message);
        this->continue_send(sp);
      }
    }

    void continue_send(const vds::service_provider & sp) {
      this->output_stream_->write_async(
          sp,
          this->messages_queue_.front())
          .execute([sp, pthis = this->shared_from_this()](const std::shared_ptr<std::exception> & ex) {
        if (!ex) {
          auto this_ = static_cast<_http_pipeline *>(pthis.get());
          std::unique_lock<std::mutex> lock(this_->messages_queue_mutex_);
          this_->messages_queue_.pop();
          if (!this_->messages_queue_.empty()) {
            this_->continue_send(sp);
          }
        }
      });
    }
  };

  class http_pipeline : public stream<uint8_t> {
  public:
    http_pipeline(
        const service_provider &sp,
        const std::shared_ptr<http_async_serializer> & output_stream,
        const std::function<async_task<http_message>(const http_message &message)> &message_callback)
        : stream<uint8_t>(new _http_pipeline(sp, message_callback, output_stream)) {
        }

    void reset() const {
      auto p = static_cast<_http_pipeline *>(this->impl_.get());
      p->reset();
    }
  };
}

#endif // __VDS_HTTP_HTTP_PIPELINE_H_
