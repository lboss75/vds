#ifndef __VDS_HTTP_HTTP_REQUEST_H_
#define __VDS_HTTP_HTTP_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

namespace vds {
  class http_parser;
  
  class http_request : public std::enable_shared_from_this<http_request>
  {
  public:
    http_request(
      const std::string & method,
      const std::string & url,
      const std::string & agent,
      const std::list<std::string> & headers);
    
    const std::string & url() const {
      return this->url_;
    }
    
    const std::string & method() const {
      return this->method_;
    }
    const std::string & agent() const {
      return this->agent_;
    }
   
  private:
    std::string method_;
    std::string url_;
    std::string agent_;
    std::list<std::string> headers_;
  };
}

#endif // __VDS_HTTP_HTTP_REQUEST_H_
