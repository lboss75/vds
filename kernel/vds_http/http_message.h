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
        const service_provider & sp,
        const std::list<std::string> & headers)
    : headers_(headers), body_(std::make_shared<continuous_buffer<uint8_t>>(sp))
    {
    }

    const std::list<std::string> & headers() const {
      return this->headers_;
    }

    bool get_header(const std::string & name, std::string & value) const;
    
    const std::shared_ptr<continuous_buffer<uint8_t>> & body() const {
      return this->body_;
    }

  private:
    std::list<std::string> headers_;
    std::shared_ptr<continuous_buffer<uint8_t>> body_;
  };
}
#endif // __VDS_HTTP_HTTP_MESSAGE_H_
