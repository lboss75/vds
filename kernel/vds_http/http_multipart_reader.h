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
      const service_provider * sp,
      const std::string & boundary,
      const std::function<std::future<void>(const http_message &message)> & message_callback)
      : sp_(sp), boundary_(boundary), message_callback_(message_callback), readed_(0), is_first_(true), eof_(false) {
    }

    std::future<void> process(
      const std::shared_ptr<stream_input_async<uint8_t>> & input_stream)
    {
      while(!this->eof_) {
        auto parser = std::make_shared<http_form_part_parser>(this->message_callback_);
        co_await parser->start(this->sp_, std::make_shared<part_reader>(this->shared_from_this(), input_stream));
      }
    }

  private:
    const service_provider * sp_;
    const std::string boundary_;
    std::function<std::future<void>(const http_message &message)> message_callback_;

    uint8_t buffer_[1024];
    size_t readed_;

    bool is_first_;
    bool eof_;

    class part_reader : public stream_input_async<uint8_t> {
    public:
      part_reader(
        const std::shared_ptr<http_multipart_reader> & owner,
        const std::shared_ptr<stream_input_async<uint8_t>> & input_stream)
      : owner_(owner), input_stream_(input_stream) {        
      }

      std::future<size_t> read_async( uint8_t * buffer, size_t len) override {
        auto pthis = this->shared_from_this();

        for (;;) {
          while (0 < this->owner_->readed_) {
            std::string value((const char *)this->owner_->buffer_, this->owner_->readed_);
            auto p = value.find(this->owner_->boundary_);
            if (std::string::npos != p) {
              if (0 < p) {
                auto finish = p;
                if (0 < finish && this->owner_->buffer_[finish - 1] == '\n') {
                  --finish;
                }
                if (0 < finish && this->owner_->buffer_[finish - 1] == '\r') {
                  --finish;
                }

                if(finish > len) {
                  finish = len;
                }
                if (0 < finish) {
                  memcpy(buffer, this->owner_->buffer_, finish);
                  memmove(this->owner_->buffer_, this->owner_->buffer_ + finish, this->owner_->readed_ - finish);
                  vds_assert(this->owner_->readed_ >= finish);
                  this->owner_->readed_ -= finish;

                  co_return finish;
                }
                else {
                  memmove(this->owner_->buffer_, this->owner_->buffer_ + p, this->owner_->readed_ - p);
                  vds_assert(this->owner_->readed_ >= p);
                  this->owner_->readed_ -= p;
                }
                continue;
              }

              if (this->owner_->readed_ > this->owner_->boundary_.length()) {
                size_t offset = 0;
                if (this->owner_->buffer_[this->owner_->boundary_.length()] == '\n') {
                  offset = 1;
                }
                else if (this->owner_->buffer_[this->owner_->boundary_.length()] == '\r'
                  && this->owner_->readed_ > this->owner_->boundary_.length() + 1
                  && this->owner_->buffer_[this->owner_->boundary_.length() + 1] == '\n') {
                  offset = 2;
                }

                if (offset > 0) {
                  this->owner_->readed_ -= this->owner_->boundary_.length();
                  vds_assert(this->owner_->readed_ >= offset);
                  this->owner_->readed_ -= offset;
                  memmove(this->owner_->buffer_, this->owner_->buffer_ + this->owner_->boundary_.length() + offset, this->owner_->readed_);

                  if (!this->owner_->is_first_) {
                    co_return 0;
                  }
                  else {
                    this->owner_->is_first_ = false;
                  }

                  continue;
                }
                else if (this->owner_->buffer_[this->owner_->boundary_.length()] == '-'
                  && this->owner_->readed_ > this->owner_->boundary_.length() + 1
                  && this->owner_->buffer_[this->owner_->boundary_.length() + 1] == '-') {
                  this->owner_->eof_ = true;
                  co_return 0;
                }
              }
            }
            else if (this->owner_->readed_ > this->owner_->boundary_.length() + 2) {
              vds_assert(!this->owner_->is_first_);
              if (len > this->owner_->readed_ - this->owner_->boundary_.length() - 2) {
                len = this->owner_->readed_ - this->owner_->boundary_.length() - 2;
              }

              vds_assert(this->owner_->readed_ >= len);

              memcpy(buffer, this->owner_->buffer_, len);
              memmove(this->owner_->buffer_, this->owner_->buffer_ + len, this->owner_->readed_ - len);

              this->owner_->readed_ -= len;

              co_return len;
            }
            break;
          }

          if (!this->owner_->eof_) {
            size_t readed = co_await this->input_stream_->read_async(
              this->owner_->buffer_ + this->owner_->readed_,
              sizeof(this->owner_->buffer_) - this->owner_->readed_);

            if (0 != readed) {
              this->owner_->readed_ += readed;
            }
            else {
              this->owner_->eof_ = true;
            }
          }

          if(this->owner_->eof_){
            if (this->owner_->readed_ > 0) {
              if(len > this->owner_->readed_) {
                len = this->owner_->readed_;
              }
              memcpy(buffer, this->owner_->buffer_, len);
              vds_assert(this->owner_->readed_ >= len);
              this->owner_->readed_ -= len;
              memmove(this->owner_->buffer_, this->owner_->buffer_ + len, this->owner_->readed_ - len);

              co_return len;
            }

            co_return 0;
          }
        }
      }
    private:
      std::shared_ptr<http_multipart_reader> owner_;
      std::shared_ptr<stream_input_async<uint8_t>> input_stream_;
    };
  };
}

#endif // __VDS_HTTP_HTTP_MULTIPART_READER_H_
