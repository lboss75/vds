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
#include "http_form_part_parser.h"
#include "vds_debug.h"

namespace vds {
  class http_multipart_reader : public std::enable_shared_from_this<http_multipart_reader> {
  public:
    http_multipart_reader(
      const service_provider & sp,
      const std::string & boundary,
      const std::function<std::future<void>(const http_message &message)> & message_callback)
      : boundary_(boundary), parser_(sp, message_callback), readed_(0), is_first_(true){
    }

    std::future<void> process(
      const service_provider & sp,
      const http_message & message)
    {
      return this->continue_read(sp, message.body());
    }

  private:
    const std::string boundary_;
    http_form_part_parser parser_;

    uint8_t buffer_[1024];
    size_t readed_;

    bool is_first_;

    std::future<void> continue_read(
      const service_provider & sp,
      const std::shared_ptr<continuous_buffer<uint8_t>> & body)
    {
      while (0 < this->readed_) {
        std::string value((const char *)this->buffer_, this->readed_);
        auto p = value.find(this->boundary_);
        if (std::string::npos != p) {
          if (0 < p) {
            auto finish = p;
            if(0 < finish && this->buffer_[finish - 1] == '\n') {
              --finish;
            }
            if (0 < finish && this->buffer_[finish - 1] == '\r') {
              --finish;
            }
            this->parser_.write(this->buffer_, finish);
            memmove(this->buffer_, this->buffer_ + p, this->readed_ - p);
            this->readed_ -= p;

            continue;
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

              if (!this->is_first_) {
                this->parser_.write(nullptr, 0);
                this->parser_.reset();
              }
              else {
                this->is_first_ = false;
              }

              this->readed_ -= this->boundary_.length();
              this->readed_ -= offset;
              memmove(this->buffer_, this->buffer_ + this->boundary_.length() + offset, this->readed_);

              continue;
            }
            else if (this->buffer_[this->boundary_.length()] == '-'
              && this->readed_ > this->boundary_.length() + 1
              && this->buffer_[this->boundary_.length() + 1] == '-') {
              if (!this->is_first_) {
                this->parser_.write(nullptr, 0);
              }
              return std::future<void>::empty();
            }
          }
        }
        else if (this->readed_ > this->boundary_.length() + 2) {
          vds_assert(!this->is_first_);
          this->parser_.write(this->buffer_, this->readed_ - this->boundary_.length() - 2);
          memmove(this->buffer_, this->buffer_ + this->readed_ - this->boundary_.length() - 2, this->boundary_.length() + 2);
          this->readed_ = this->boundary_.length() + 2;

          continue;
        }
        break;
      }
      return body->read_async(this->buffer_ + this->readed_, sizeof(this->buffer_) - this->readed_)
        .then([pthis = this->shared_from_this(), sp, body](size_t readed) {
        if (0 != readed) {
          pthis->readed_ += readed;
          return pthis->continue_read(sp, body);
        }
        else {
          if (pthis->readed_ > 0) {
            pthis->parser_.write(pthis->buffer_, pthis->readed_);
          }

          return std::future<void>::empty();
        }
      });
    }
  };
}

#endif // __VDS_HTTP_HTTP_MULTIPART_READER_H_
