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
        stream_async<uint8_t> & target)
        : target_(target)
      {
      }
      
      ~http_async_serializer()
      {
      }

      async_task<> write_async(
        const service_provider & sp,
        const http_message & message)
      {
        vds_assert(message.body());
        this->buffer_ = std::make_shared<continuous_buffer<uint8_t>>(sp);

        std::stringstream stream;
        for (auto & header : message.headers()) {
          stream << header << "\r\n";
        }
        sp.get<logger>()->trace("HTTP", sp, "HTTP Send [%s]", logger::escape_string(stream.str()).c_str());
        stream << "\r\n";

        auto data = std::make_shared<std::string>(stream.str());
        
        return async_series(
          this->buffer_->write_async((const uint8_t *)data->c_str(), data->length())
            .then([pthis = this->shared_from_this(), sp, data, message]() {
              auto buffer = std::make_shared<std::vector<uint8_t>>(1024);
              return pthis->write_body(sp, message, buffer);
            }),
          this->continue_process(sp));
      }

    private:
      stream_async<uint8_t> & target_;
      std::shared_ptr<continuous_buffer<uint8_t>> buffer_;
      uint8_t output_buffer_[1024];

      async_task<> write_body(
        const service_provider & sp,
        const http_message & message,
        const std::shared_ptr<std::vector<uint8_t>> & buffer)
      {
        return message.body()->read_async(buffer->data(), buffer->size())
          .then([pthis = this->shared_from_this(), sp, message, buffer](size_t readed) {
          if (0 < readed) {
            sp.get<logger>()->trace("HTTP", sp, "HTTP Send [%s]", std::string((const char *)buffer->data(), readed).c_str());

            return pthis->buffer_->write_async(buffer->data(), readed)
            .then(
              [pthis, sp, message, buffer]() {
                return pthis->write_body(sp, message, buffer);
            });
          }
          else {
            auto buffer = pthis->buffer_;
            return buffer->write_async(nullptr, 0);
          }
        });
      }

      async_task<> continue_process(const service_provider & sp)
      {
        return this->buffer_->read_async(this->output_buffer_, sizeof(this->output_buffer_))
          .then([pthis = this->shared_from_this(), sp](size_t readed) {
            if (0 < readed) {
              return 
                pthis->target_.write_async(pthis->output_buffer_, readed)
                .then([pthis, sp]() {
                    return pthis->continue_process(sp);
                  });
            }
            else {
              return pthis->target_.write_async(nullptr, 0)
                .then([]() {
                  return async_task<>::empty();
                });
            }
          });
      }
  };

  class http_serializer : public std::enable_shared_from_this<http_serializer>
  {
  public:
    http_serializer(
      stream<uint8_t> & target)
      : target_(target)
    {
    }

    ~http_serializer()
    {
    }

    async_task<> write_async(
      const service_provider & sp,
      const http_message & message)
    {
      this->buffer_ = std::make_shared<continuous_buffer<uint8_t>>(sp);

      std::stringstream stream;
      for (auto & header : message.headers()) {
        stream << header << "\r\n";
      }
      sp.get<logger>()->trace("HTTP", sp, "HTTP Send [%s]", logger::escape_string(stream.str()).c_str());
      stream << "\r\n";

      auto data = std::make_shared<std::string>(stream.str());

      return async_series(
        this->buffer_->write_async((const uint8_t *)data->c_str(), data->length())
        .then([pthis = this->shared_from_this(), sp, data, message]() {
        auto buffer = std::make_shared<std::vector<uint8_t>>(1024);
        return pthis->write_body(sp, message.body(), buffer);
      }),
        this->continue_process(sp));
    }

  private:
    stream<uint8_t> & target_;
    std::shared_ptr<continuous_buffer<uint8_t>> buffer_;
    uint8_t output_buffer_[1024];

    async_task<> write_body(
      const service_provider & sp,
      const std::shared_ptr<continuous_buffer<uint8_t>> & body,
      const std::shared_ptr<std::vector<uint8_t>> & buffer)
    {
      return body->read_async(buffer->data(), buffer->size())
        .then([pthis = this->shared_from_this(), sp, body, buffer](size_t readed) {
        if (0 < readed) {
          sp.get<logger>()->trace("HTTP", sp, "HTTP Send [%s]", std::string((const char *)buffer->data(), readed).c_str());

          return pthis->buffer_->write_async(buffer->data(), readed)
            .then(
              [pthis, sp, body, buffer]() {
            pthis->write_body(sp, body, buffer);
          });
        }
        else {
          auto buffer = pthis->buffer_;
          return buffer->write_async(nullptr, 0);
        }
      });
    }

    async_task<> continue_process(const service_provider & sp)
    {
      return this->buffer_->read_async(this->output_buffer_, sizeof(this->output_buffer_))
        .then([pthis = this->shared_from_this(), sp](size_t readed) {
        if (0 < readed) {
          pthis->target_.write(pthis->output_buffer_, readed);
          return pthis->continue_process(sp);
        }
        else {
          pthis->target_.write(nullptr, 0);
          return async_task<>::empty();
        }
      });
    }
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_SERIALIZER_H_
