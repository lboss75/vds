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

namespace vds {
  class http_parser
  {
  public:
    http_parser(
      const std::function<void(const service_provider & sp, const std::shared_ptr<http_message> & message)> & message_callback
    ) : message_callback_(message_callback)
    {
    }

    using incoming_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public async_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = async_dataflow_target<context_type, handler<context_type>>;

    public:
      handler(
        const context_type & context,
        const http_parser & args)
        : base_class(context),
        message_callback_(args.message_callback_),
        state_(StateEnum::STATE_PARSE_HEADER)
      {
      }

      void async_push_data(const service_provider & sp)
      {
        this->continue_push_data(sp, 0);

      }

    private:
      std::function<void(const service_provider & sp, const std::shared_ptr<http_message> & message)> message_callback_;
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

      void continue_push_data(const service_provider & sp, size_t readed)
      {
        while (readed < this->input_buffer_size()) {
          if (StateEnum::STATE_PARSE_HEADER == this->state_) {
            char * p = (char *)memchr((const char *)this->input_buffer() + readed, '\n', this->input_buffer_size() - readed);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *)this->input_buffer() + readed, this->input_buffer_size() - readed);
              if (!this->processed(sp, this->input_buffer_size())) {
                return;
              }
              readed = 0;
              continue;
            }

            auto size = p - (const char *)this->input_buffer() - readed;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(this->input_buffer())[size + readed - 1]) {
                this->parse_buffer_ += std::string((const char *)this->input_buffer() + readed, size - 1);
              }
              else {
                this->parse_buffer_.append((const char *)this->input_buffer() + readed, size);
              }
            }
            readed += size + 1;

            if (0 == this->parse_buffer_.length()) {
              if (this->headers_.empty()) {
                throw std::logic_error("Invalid request");
              }

              auto current_message = std::make_shared<http_message>(this->headers_);
              mt_service::async(sp, [sp, this, current_message]() {
                this->message_callback_(sp, current_message);
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
            auto size = this->input_buffer_size() - readed;
            if (size > this->content_length_) {
              size = this->content_length_;
            }

            if (0 < size) {
              this->content_length_ -= size;
              this->current_message_->body()->write_all_async(
                sp,
                this->input_buffer() + readed,
                size)
                .wait(
                  [this, readed = readed + size](const service_provider & sp) {

                if (0 == this->content_length_) {
                  this->state_ = StateEnum::STATE_PARSE_HEADER;
                  this->current_message_->body()->write_all_async(
                    sp,
                    nullptr,
                    0)
                    .wait(
                      [this, readed](const service_provider & sp) {
                    if (this->processed(sp, readed)) {
                      this->continue_push_data(sp, 0);
                    }
                  },
                      [this](const service_provider & sp, std::exception_ptr ex) {
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
                  [this](const service_provider & sp, std::exception_ptr ex) {
                this->error(sp, ex);
              },
                sp);
              return;
            }
          }
        }

        this->processed(sp, readed);
      }
    };
  private:
    std::function<void(const service_provider & sp, const std::shared_ptr<http_message> & message)> message_callback_;

  };
}

#endif // HTTP_PARSER_H
