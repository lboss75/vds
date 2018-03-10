#ifndef __VDS_HTTP_HTTP_MULTIPART_READER_H_
#define __VDS_HTTP_HTTP_MULTIPART_READER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>
#include "http_message.h"
#include "http_parser.h"

namespace vds {
  class http_multipart_reader : public std::enable_shared_from_this<http_multipart_reader> {
  public:
    http_multipart_reader(
      const service_provider & sp,
      const std::string & boundary,
      const std::function<async_task<>(const http_message &message)> & message_callback)
      : boundary_(boundary), parser_(sp, message_callback), readed_(0) {
    }

    async_task<> process(
      const service_provider & sp,
      const http_message & message)
    {
      return this->continue_read(sp, message);
    }

  private:
    const std::string boundary_;

    uint8_t buffer_[1024];
    size_t readed_;

    http_parser parser_;

    async_task<> continue_read(
      const service_provider & sp,
      const http_message & message)
    {
      if (0 < this->readed_) {
        std::string value((const char *)this->buffer_, this->readed_);
        auto p = value.find(this->boundary_);
        if (std::string::npos != p) {
          if (0 < p) {
            this->parser_.write(this->buffer_, p);
            memmove(this->buffer_, this->buffer_ + p, this->readed_ - p);
            this->readed_ -= p;

            return this->continue_read(sp, message);
          }

          if (this->readed_ > this->boundary_.length()) {
            size_t offset = 0;
            if (this->buffer_[this->boundary_.length()] == '\n') {
              offset = 1;
            }
            else if (this->buffer_[this->boundary_.length()] == '\r'
              && this->readed_ > this->boundary_.length() + 1
              && this->buffer_[this->boundary_.length() + 1] == '\n') {
              offset = 2;
            }

            if (offset > 0) {

              this->parser_.reset();

              this->readed_ -= this->boundary_.length();
              this->readed_ -= offset;
              memmove(this->buffer_, this->buffer_ + this->boundary_.length() + offset, this->readed_);

              return this->continue_read(sp, message);
            }
          }
          else if (this->readed_ > this->boundary_.length()) {
            this->parser_.write(this->buffer_, this->readed_ - this->boundary_.length());
            memmove(this->buffer_, this->buffer_ + this->readed_ - this->boundary_.length(), this->boundary_.length());
            this->readed_ = this->boundary_.length();

            return this->continue_read(sp, message);
          }
        }
      }
      return message.body()->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_)
        .then([pthis = this->shared_from_this(), sp, message](size_t readed) {
        if (0 != readed) {
          pthis->readed_ += readed;
          return pthis->continue_read(sp, message);
        }
        else {
          if (pthis->readed_ > 0) {
            pthis->parser_.write(pthis->buffer_, pthis->readed_);
          }

          return async_task<>::empty();
        }
      });
    }
  };
}

#endif // __VDS_HTTP_HTTP_MULTIPART_READER_H_
