#ifndef __VDS_HTTP_HTTP_MESSAGE_H_
#define __VDS_HTTP_HTTP_MESSAGE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

namespace vds {
  class http_message
  {
  public:
    http_message(const std::list<std::string> & headers, const std::string & body)
    : headers_(headers), body_(body)
    {
    }
    
    const std::list<std::string> & headers() {
      return this->headers_;
    }

    bool get_header(const std::string & name, std::string & value);
    
    const std::string & body() const {
      return this->body_;
    }

    
  private:
    std::list<std::string> headers_;
    std::string body_;
  };
}
#endif // __VDS_HTTP_HTTP_MESSAGE_H_
