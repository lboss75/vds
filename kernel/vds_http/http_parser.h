#ifndef __VDS_HTTP_HTTP_PARSER_H
#define __VDS_HTTP_HTTP_PARSER_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <list>
#include <string.h>

#include "func_utils.h"
#include "service_provider.h"
#include "http_message.h"
#include "barrier.h"

namespace vds {
  class http_parser : public stream<uint8_t>, public std::enable_shared_from_this<http_parser>
  {
  public:
    static std::shared_ptr<http_parser> create(
      const service_provider & sp,
      const std::function<async_task<>(const service_provider & sp, const std::shared_ptr<http_message> & message)> & message_callback)
    {
      return std::make_shared<http_parser>(sp, message_callback);
    }

      void write(const uint8_t * data, size_t len) override
      {
        this->sp_.get<logger>()->debug("HTTP", this->sp_, "HTTP [%s]", logger::escape_string(std::string((const char *)data, len)).c_str());
        this->continue_push_data(data, len);
      }
      
      void finish() override
      {
          this->sp_.get<logger>()->debug("HTTP", this->sp_, "HTTP end");
          
          this->message_callback_(this->sp_, std::shared_ptr<http_message>())
          .wait(
            [](){ },
            [pthis = this->shared_from_this()](const std::shared_ptr<std::exception> & ex){
              pthis->sp_.unhandled_exception(ex);
            });
      }

    private:
      http_parser(
        const service_provider & sp,
        const std::function<async_task<>(const service_provider & sp, const std::shared_ptr<http_message> & message)> & message_callback
      ) : sp_(sp),
          message_callback_(message_callback),
          message_body_barrier_(0),
          state_(StateEnum::STATE_PARSE_HEADER)
      {
      }
      
      service_provider sp_;
      std::function<async_task<>(const service_provider & sp, const std::shared_ptr<http_message> & message)> message_callback_;
      std::shared_ptr<std::exception> error_;
      barrier message_barrier_;
      barrier message_body_barrier_;
      
      std::string parse_buffer_;
      std::list<std::string> headers_;
      std::shared_ptr<http_message> current_message_;

      enum class StateEnum
      {
        STATE_PARSE_HEADER,
        STATE_PARSE_BODY
      };
      StateEnum state_;

      size_t content_length_;

      void continue_push_data(const uint8_t * data, size_t len)
      {
        while (0 < len) {
          if (StateEnum::STATE_PARSE_HEADER == this->state_) {
            char * p = (char *)memchr((const char *)data, '\n', len);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)data, len);
              return;
            }

            auto size = p - (const char *)data;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(data)[size - 1]) {
                this->parse_buffer_ += std::string((const char *)data, size - 1);
              }
              else {
                this->parse_buffer_.append((const char *)data, size);
              }
            }
            
            data += size + 1;
            len -= size + 1;

            if (0 == this->parse_buffer_.length()) {
              if (this->headers_.empty()) {
                throw std::logic_error("Invalid request");
              }

              auto current_message = std::make_shared<http_message>(this->headers_);
              mt_service::async(this->sp_, [pthis = this->shared_from_this(), current_message]() {
                pthis->message_callback_(pthis->sp_, current_message).wait(
                  [pthis](){
                    pthis->message_barrier_.set();
                  },
                  [pthis](const std::shared_ptr<std::exception> & ex){
                    pthis->error_ = ex;
                    pthis->message_barrier_.set();
                  });
              });
              this->current_message_ = current_message;
              this->headers_.clear();

              std::string content_length_header;
              if (this->current_message_->get_header("Content-Length", content_length_header)) {
                this->content_length_ = std::stoul(content_length_header);
              }
              else {
                this->content_length_ = 0;
              }

              if (0 < this->content_length_) {
                this->state_ = StateEnum::STATE_PARSE_BODY;
              }
            }
            else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }
          }
          else {
            auto size = len;
            if (size > this->content_length_) {
              size = this->content_length_;
            }

            if (0 < size) {
              this->content_length_ -= size;
              this->current_message_->body()->write_all_async(
                this->sp_,
                data,
                size)
                .wait(
                  [pthis = this->shared_from_this(), readed = readed + size](const service_provider & sp) {

                if (0 == pthis->content_length_) {
                  this->state_ = StateEnum::STATE_PARSE_HEADER;
                  this->current_message_->body()->write_all_async(
                    sp,
                    nullptr,
                    0)
                    .wait(
                      [this, readed](const service_provider & sp) {
                    this->message_barrier_.wait();
                    this->message_barrier_.reset();
                    if(this->error_){
                      this->error(sp, this->error_);
                    }
                    else if (this->processed(sp, readed)) {
                      this->continue_push_data(sp, 0);
                    }
                  },
                      [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                    this->error(sp, ex);
                  },
                    sp);
                }
                else {
                  if (this->processed(sp, readed)) {
                    this->continue_push_data(sp, 0);
                  }
                }
              },
                  [this](const service_provider & sp, const std::shared_ptr<std::exception> & ex) {
                this->error(sp, ex);
              });
              return;
            }
          }
        }

        this->processed(sp, readed);
      }
  };
}

#endif // HTTP_PARSER_H
