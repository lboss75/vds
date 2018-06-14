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

  template<typename implementation_class>
  class _http_parser_base : public _stream<uint8_t> {
  public:
    _http_parser_base(
        const service_provider &sp,
        const std::function<async_task<>(const http_message &message)> &message_callback)
        : sp_(sp),
          message_callback_(message_callback),
          message_state_(std::make_shared<state_machine<MessageStateEnum>>(MessageStateEnum::MESSAGE_STATE_NONE)),
          state_(StateEnum::STATE_PARSE_HEADER) {
    }

    void write(
        const uint8_t *data,
        size_t len) override {
      if (0 == len) {
        if (StateEnum::STATE_PARSE_SIZE == this->state_ && this->expect_100_) {
          static_cast<implementation_class *>(this)->continue_read_data(this->sp_);
          return;
        }
        auto pthis = this->shared_from_this();
        logger::get(this->sp_)->debug("HTTP", this->sp_, "HTTP end");

        logger::get(this->sp_)->trace("HTTP", this->sp_, "State: NONE -> MESSAGE_STARTED");

        this->message_state_->change_state(
                MessageStateEnum::MESSAGE_STATE_NONE,
                MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED)
        .then([pthis]() {
          auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
          return this_->message_callback_(http_message());
        })
        .then([pthis]() {
          auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
          logger::get(this_->sp_)->trace("HTTP", this_->sp_, "State: MESSAGE_STARTED -> NONE");
          return this_->message_state_->change_state(
              MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
              MessageStateEnum::MESSAGE_STATE_NONE);
        })
        .then([pthis]() {
          auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
          this_->message_state_->wait(MessageStateEnum::MESSAGE_STATE_NONE).wait();
        })
        .wait();
      } else {

        logger::get(this->sp_)->debug("HTTP", this->sp_, "HTTP Parse [%s]",
                                       logger::escape_string(std::string((const char *) data, len)).c_str());

        this->continue_push_data(data, len);
      }
    }

    void reset() {
      this->message_state_->wait(MessageStateEnum::MESSAGE_STATE_NONE).wait();
      this->state_ = StateEnum::STATE_PARSE_HEADER;
    }

    void continue_read_data(const service_provider &sp) {
    }

    void finish_message(const service_provider &sp) {
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
    std::shared_ptr<state_machine<MessageStateEnum>> message_state_;

    std::string parse_buffer_;
    std::list<std::string> headers_;
    http_message current_message_;

    enum class StateEnum {
      STATE_PARSE_HEADER,
      STATE_PARSE_BODY,
      STATE_PARSE_SIZE,
      STATE_PARSE_FINISH_CHUNK
    };
    StateEnum state_;

    size_t content_length_;
    bool chunked_encoding_;
    bool expect_100_;

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
              if (this->headers_.empty()) {
                throw std::logic_error("Invalid request");
              }

              http_message current_message(this->sp_, this->headers_);

              std::string transfer_encoding;
              this->chunked_encoding_ = (current_message.get_header("Transfer-Encoding", transfer_encoding) &&
                                         transfer_encoding == "chunked");

              std::string expect_value;
              this->expect_100_ = (current_message.get_header("Expect", expect_value) &&
                                   "100-continue" == expect_value);

              logger::get(this->sp_)->trace("HTTP", this->sp_, "State: Node -> MESSAGE_STARTED");
              this->message_state_->change_state(
                  MessageStateEnum::MESSAGE_STATE_NONE,
                  MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED).wait();

              mt_service::async(this->sp_, [pthis, current_message]() {
                auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
                this_->message_callback_(current_message)
                    .then([pthis]() {
                      auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
                      logger::get(this_->sp_)->trace("HTTP", this_->sp_, "State: MESSAGE_BODY_FINISH -> None");
                      return this_->message_state_->change_state(
                          MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH,
                          MessageStateEnum::MESSAGE_STATE_NONE);
                    })
                    .execute([sp = this_->sp_](const std::shared_ptr<std::exception> &ex) {
                  if(ex) {
                    sp.unhandled_exception(ex);
                  }
                });
              });
              this->current_message_ = current_message;
              this->headers_.clear();

              std::string content_length_header;
              if (this->current_message_.get_header("Content-Length", content_length_header)) {
                this->content_length_ = std::stoul(content_length_header);
              } else if(current_message.get_header("Transfer-Encoding", transfer_encoding)) {
                this->content_length_ = (size_t) -1;
              }
              else {
                this->content_length_ = 0;
              }

              if(0 == this->content_length_) {
                this->current_message_.body()->write_async(nullptr, 0)
                  .then([pthis]() {
                    auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
                    logger::get(this_->sp_)->trace("HTTP", this_->sp_, "State: MESSAGE_STARTED -> MESSAGE_BODY_FINISH");
                    return this_->message_state_->change_state(
                      MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                      MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH);
                  })
                  .execute([](const std::shared_ptr<std::exception> &ex) {});
                static_cast<implementation_class *>(this)->finish_message(this->sp_);
              }

              if (this->expect_100_) {
                static_cast<implementation_class *>(this)->continue_read_data(this->sp_);
              }

              if (0 < this->content_length_) {
                if (this->chunked_encoding_) {
                  this->state_ = StateEnum::STATE_PARSE_SIZE;
                  this->parse_buffer_.clear();
                } else {
                  this->state_ = StateEnum::STATE_PARSE_BODY;
                }
              }
            } else {
              this->headers_.push_back(this->parse_buffer_);
              this->parse_buffer_.clear();
            }

            break;
          }
          case StateEnum::STATE_PARSE_BODY: {
            auto size = len;
            if (size > this->content_length_) {
              size = this->content_length_;
            }

            if (0 < size) {
              this->content_length_ -= size;
              this->current_message_.body()->write_async(data, size)
                  .wait();

              data += size;
              len -= size;

              if (0 == this->content_length_) {
                if (this->chunked_encoding_) {
                  this->state_ = StateEnum::STATE_PARSE_FINISH_CHUNK;
                  this->parse_buffer_.clear();
                } else {
                  static_cast<implementation_class *>(this)->finish_message(this->sp_);
                  this->state_ = StateEnum::STATE_PARSE_HEADER;
                  this->current_message_.body()->write_async(nullptr, 0)
                      .then([pthis]() {
                            auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
                              logger::get(this_->sp_)->trace("HTTP", this_->sp_, "State: MESSAGE_STARTED -> MESSAGE_BODY_FINISH");
                              return this_->message_state_->change_state(
                                  MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                                  MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH);
                          })
                  .execute([](const std::shared_ptr<std::exception> &ex) {});
                }
              }
            }
            break;
          }
          case StateEnum::STATE_PARSE_SIZE: {
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

            this->content_length_ = 0;
            for (auto ch : this->parse_buffer_) {
              if ('0' <= ch && ch <= '9') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - '0';
              } else if ('a' <= ch && ch <= 'f') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - 'a' + 10;
              } else if ('A' <= ch && ch <= 'F') {
                this->content_length_ <<= 4;
                this->content_length_ |= ch - 'A' + 10;
              } else {
                throw std::runtime_error("Invalid size " + this->parse_buffer_);
              }
            }

            if (0 < this->content_length_) {
              this->state_ = StateEnum::STATE_PARSE_BODY;
            } else {
              this->state_ = StateEnum::STATE_PARSE_HEADER;
              this->current_message_.body()->write_async(nullptr, 0)
                  .then(
                      [pthis]() {
                        auto this_ = static_cast<_http_parser_base<implementation_class> *>(pthis.get());
                        logger::get(this_->sp_)->trace("HTTP", this_->sp_, "State: MESSAGE_STARTED -> MESSAGE_BODY_FINISH");
                        return this_->message_state_->change_state(
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_STARTED,
                            MessageStateEnum::MESSAGE_STATE_MESSAGE_BODY_FINISH);
                      })
              .execute([](const std::shared_ptr<std::exception> &ex) {});
            }
            break;
          }
          case StateEnum::STATE_PARSE_FINISH_CHUNK: {
            if (data[0] == '\r' && this->parse_buffer_.empty()) {
              this->parse_buffer_ += '\r';
              ++data;
              --len;
            } else if (data[0] == '\n') {
              ++data;
              --len;
              this->state_ = StateEnum::STATE_PARSE_SIZE;
              static_cast<implementation_class *>(this)->continue_read_data(this->sp_);
            } else {
              throw std::runtime_error("Invalid data");
            }

            break;
          }
          default: {
            throw std::runtime_error("Invalid state");
          }
        }
      }
    }
  };

  class _http_parser : public _http_parser_base<_http_parser> {
  public:
    _http_parser(
        const service_provider &sp,
        const std::function<async_task<>(const http_message &message)> &message_callback)
        : _http_parser_base<_http_parser>(sp, message_callback) {
    }
  };

  class http_parser : public stream<uint8_t> {
  public:
    http_parser(
        const service_provider &sp,
        const std::function<async_task<>(const http_message &message)> &message_callback)
        : stream<uint8_t>(new _http_parser(sp, message_callback)) {
    }

    void reset() const {
      auto p = static_cast<_http_parser *>(this->impl_.get());
      p->reset();
    }

    static std::string url_unescape(const std::string & string);
    static std::string url_escape(const std::string & string);
  };
}

#endif // __VDS_HTTP_HTTP_PARSER_H

