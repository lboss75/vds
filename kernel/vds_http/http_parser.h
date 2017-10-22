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
#include "state_machine.h"

namespace vds {
class http_parser : public stream<uint8_t>, public std::enable_shared_from_this<http_parser>
{
public:
    static std::shared_ptr<http_parser> create(
        const std::function<async_task<>(const std::shared_ptr<http_message> & message)> & message_callback)
    {
        return std::make_shared<http_parser>(message_callback);
    }

    void write(
      const service_provider & sp,
      const uint8_t * data,
      size_t len) override
    {
      if(0 == len){
         auto pthis = this->shared_from_this();
        sp.get<logger>()->debug("HTTP", sp, "HTTP end");

        this->message_state_.change_state(
          MessageStateEnum::MESSAGE_STATE_NONE,
          MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
          error_logic::throw_exception);
        this->message_callback_(std::shared_ptr<http_message>())
        .wait(
          [pthis]() {
              pthis->message_state_.change_state(
                  MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                  MessageStateEnum::MESSAGE_STATE_NONE,
                  error_logic::return_false);
          },
          [pthis](const std::shared_ptr<std::exception> & ex) {
              pthis->message_state_.fail(ex);
          });
        this->message_state_.wait(MessageStateEnum::MESSAGE_STATE_NONE, error_logic::throw_exception);
      }
      else {
        sp.get<logger>()->debug("HTTP", sp, "HTTP [%s]", logger::escape_string(std::string((const char *)data, len)).c_str());
        this->continue_push_data(sp, data, len);
      }
    }

private:
    http_parser(
        const std::function<async_task<>(const std::shared_ptr<http_message> & message)> & message_callback)
    : message_callback_(message_callback),
      message_state_(MessageStateEnum::MESSAGE_STATE_NONE, MessageStateEnum::MESSAGE_STATE_FAILED),
      state_(StateEnum::STATE_PARSE_HEADER)
    {
    }

    std::function<async_task<>(const std::shared_ptr<http_message> & message)> message_callback_;

    enum class MessageStateEnum
    {
        MESSAGE_STATE_NONE,
        MESSAGE_STATE_FAILED,
        MESSAGE_STATE_MESSAGE_STARTED,
        MESSAGE_STATE_MESSAGE_BODY_STARTED,
        MESSAGE_STATE_MESSAGE_BODY_FINISH
    };
    state_machine<MessageStateEnum> message_state_;

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

    void continue_push_data(const service_provider & sp, const uint8_t * data, size_t len)
    {
      auto pthis = this->shared_from_this();
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
                    
                    this->message_state_.change_state(
                      MessageStateEnum::MESSAGE_STATE_NONE,
                      MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                      error_logic::throw_exception);

                    mt_service::async(sp, [pthis, current_message]() {
                        pthis->message_callback_(current_message).wait(
                        [pthis]() {
                            pthis->message_state_.change_state(
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                                MessageStateEnum::MESSAGE_STATE_NONE,
                                error_logic::return_false);
                        },
                        [pthis](const std::shared_ptr<std::exception> & ex) {
                            pthis->message_state_.fail(ex);
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
                        this->message_state_.change_state(
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                                error_logic::throw_exception);
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
                    this->message_state_.change_state(
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_STARTED,
                            error_logic::throw_exception);
                    
                    this->current_message_->body()->write_async(
                        sp,
                        data,
                        size)
                    .wait(
                    [pthis]() {
                      pthis->message_state_.change_state(
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_STARTED,
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                            error_logic::return_false);
                    },
                    [pthis](const std::shared_ptr<std::exception> & ex) {
                      pthis->message_state_.fail(ex);
                    });

                    data += size;
                    len -= size;

                    if (0 == this->content_length_) {
                        this->state_ = StateEnum::STATE_PARSE_HEADER;
                          this->message_state_.change_state(
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_STARTED,
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                                error_logic::throw_exception);
                        this->current_message_->body()->write_async(sp, nullptr, 0)
                        .wait(
                        [pthis]() {
                          pthis->message_state_.change_state(
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_STARTED,
                                MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                                error_logic::return_false);
                        },
                        [pthis](const std::shared_ptr<std::exception> & ex) {
                          pthis->message_state_.fail(ex);
                        });
                    }
                }
            }
        }
    }
};
}

#endif // __VDS_HTTP_HTTP_PARSER_H

