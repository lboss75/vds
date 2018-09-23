#ifndef __VDS_HTTP_HTTP_MESSAGE_H_
#define __VDS_HTTP_HTTP_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

#include "filename.h"
#include "async_buffer.h"

namespace vds {
  class http_message
  {
  public:
    http_message() {
    }

    http_message(
        const std::list<std::string> & headers,
        const std::shared_ptr<input_stream_async<uint8_t>> & body)
    : headers_(headers), body_(body)
    {
    }

    const std::list<std::string> & headers() const {
      return this->headers_;
    }

    bool get_header(const std::string & name, std::string & value) const;
    
    const std::shared_ptr<input_stream_async<uint8_t>> & body() const {
      return this->body_;
    }

    operator bool () const {
      return this->body_.get() != nullptr;
    }

    async_task<void> ignore_empty_body(const service_provider &sp) const;
    async_task<void> ignore_body(const service_provider &sp) const;

  private:
    std::list<std::string> headers_;
    std::shared_ptr<input_stream_async<uint8_t>> body_;

    struct buffer_t {
      uint8_t data_[1024];
    };
    static vds::async_task<void> ignore_body(
      const service_provider &sp,
      const std::shared_ptr<input_stream_async<uint8_t>> & body,
      const std::shared_ptr<buffer_t> & buffer);

  };
}
#endif // __VDS_HTTP_HTTP_MESSAGE_H_
