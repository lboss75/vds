#ifndef __VDS_HTTP_HTTP_FORM_PART_PARSER_H
#define __VDS_HTTP_HTTP_FORM_PART_PARSER_H

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
#include "state_machine.h"

namespace vds {

  class _http_form_part_parser : public _stream<uint8_t> {
  public:
    _http_form_part_parser(
        const service_provider &sp,
        const std::function<async_task<>(const http_message &message)> &message_callback)
        : sp_(sp),
          message_callback_(message_callback),
          message_state_(MessageStateEnum::MESSAGE_STATE_NONE, MessageStateEnum::MESSAGE_STATE_FAILED),
          state_(StateEnum::STATE_PARSE_HEADER) {
    }

    void write(
        const uint8_t *data,
        size_t len) override {
      if (0 == len) {
        auto pthis = this->shared_from_this();
        logger::get(this->sp_)->debug("HTTP", this->sp_, "HTTP end");

        this->current_message_.body()->write_async(nullptr, 0)
          .execute(
            [pthis](const std::shared_ptr<std::exception> &ex) {
          auto this_ = static_cast<_http_form_part_parser *>(pthis.get());
          if (!ex) {
            this_->message_state_.change_state(
              MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
              MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
              error_logic::return_false);
          }
          else {
            this_->message_state_.fail(ex);
          }
        });

        this->message_state_.wait(MessageStateEnum::MESSAGE_STATE_NONE, error_logic::throw_exception, std::chrono::seconds(600));
        this->state_ = StateEnum::STATE_PARSE_HEADER;
      } else {

        logger::get(this->sp_)->debug("HTTP", this->sp_, "HTTP [%s]",
                                       logger::escape_string(std::string((const char *) data, len)).c_str());

        this->continue_push_data(data, len);
      }
    }

    void reset() {
      this->message_state_.wait(MessageStateEnum::MESSAGE_STATE_NONE, error_logic::throw_exception);
      this->state_ = StateEnum::STATE_PARSE_HEADER;
    }

  private:
    service_provider sp_;
    std::function<async_task<>(const http_message &message)> message_callback_;

    enum class MessageStateEnum {
      MESSAGE_STATE_NONE,
      MESSAGE_STATE_MESSAGE_STARTED,
      MESSAGE_STATE_MESSAGE_BODY_FINISH,
      MESSAGE_STATE_FAILED
    };
    state_machine<MessageStateEnum> message_state_;

    std::string parse_buffer_;
    std::list<std::string> headers_;
    http_message current_message_;

    enum class StateEnum {
      STATE_PARSE_HEADER,
      STATE_PARSE_BODY
    };
    StateEnum state_;

    void continue_push_data(const uint8_t *data, size_t len) {
      auto pthis = this->shared_from_this();
      while (0 < len) {
        switch (this->state_) {
          case StateEnum::STATE_PARSE_HEADER: {
            char *p = (char *) memchr((const char *) data, '\n', len);
            if (nullptr == p) {
              this->parse_buffer_ += std::string((const char *) data, len);
              return;
            }

            auto size = p - (const char *) data;

            if (size > 0) {
              if ('\r' == reinterpret_cast<const char *>(data)[size - 1]) {
                this->parse_buffer_ += std::string((const char *) data, size - 1);
              } else {
                this->parse_buffer_.append((const char *) data, size);
              }
            }

            data += size + 1;
            len -= size + 1;

            if (0 == this->parse_buffer_.length()) {
              http_message current_message(this->sp_, this->headers_);

              this->message_state_.change_state(
                  MessageStateEnum::MESSAGE_STATE_NONE,
                  MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                  error_logic::throw_exception);

              mt_service::async(this->sp_, [pthis, current_message]() {
                auto this_ = static_cast<_http_form_part_parser *>(pthis.get());
                this_->message_callback_(current_message).execute(
                    [pthis](const std::shared_ptr<std::exception> &ex) {
                      auto this_ = static_cast<_http_form_part_parser *>(pthis.get());
                      if (!ex) {
                        this_->message_state_.change_state(
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                            MessageStateEnum::MESSAGE_STATE_NONE,
                            error_logic::return_false);
                      } else {
                        this_->message_state_.fail(ex);
                      }
                    });
              });
              this->current_message_ = current_message;
              this->headers_.clear();

              this->state_ = StateEnum::STATE_PARSE_BODY;
            } else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }

            break;
          }
          case StateEnum::STATE_PARSE_BODY: {
            this->current_message_.body()->write_async(
                    data,
                    len)
                .execute(
                    [pthis](const std::shared_ptr<std::exception> &ex) {
                      auto this_ = static_cast<_http_form_part_parser *>(pthis.get());
                      if (!ex) {
                      } else {
                        this_->message_state_.fail(ex);
                      }
                    });

            len = 0;
            break;
          }
          default: {
            throw std::runtime_error("Invalid state");
          }
        }
      }
    }
  };

  class http_form_part_parser : public stream<uint8_t> {
  public:
    http_form_part_parser(
        const service_provider &sp,
        const std::function<async_task<>(const http_message &message)> &message_callback)
        : stream<uint8_t>(new _http_form_part_parser(sp, message_callback)) {
    }

    void reset() const {
      auto p = static_cast<_http_form_part_parser *>(this->impl_.get());
      p->reset();
    }
  };
}

#endif // __VDS_HTTP_HTTP_FORM_PART_PARSER_H

