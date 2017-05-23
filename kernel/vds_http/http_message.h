#ifndef __VDS_HTTP_HTTP_MESSAGE_H_
#define __VDS_HTTP_HTTP_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

#include "filename.h"
#include "async_stream.h"

namespace vds {
  class http_message
  {
  public:
    http_message(const std::list<std::string> & headers)
    : headers_(headers)
    {
    }

    const std::list<std::string> & headers() {
      return this->headers_;
    }

    bool get_header(const std::string & name, std::string & value);
    
    async_stream<uint8_t> & body() {
      return this->body_;
    }

  private:
    std::list<std::string> headers_;
    async_stream<uint8_t> body_;
  };
}
#endif // __VDS_HTTP_HTTP_MESSAGE_H_
