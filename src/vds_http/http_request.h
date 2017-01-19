#ifndef __VDS_HTTP_HTTP_REQUEST_H_
#define __VDS_HTTP_HTTP_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>

namespace vds {
  class http_request
  {
  public:
    http_request()
    {

    }

    http_request(
      const std::string & url,
      const std::string & method,
      const std::string & agent = "HTTP/1.0"
      ): url_(url), method_(method), agent_(agent)
    {

    }

    void reset(
      const std::string & url,
      const std::string & method,
      const std::string & agent,
      const std::list<std::string> & headers 
    )
    {
      this->url_ = url;
      this->method_ = method;
      this->agent_ = agent;
      this->headers_ = headers;
    }

    void clear()
    {
      this->url_.clear();
      this->method_.clear();
      this->agent_.clear();
      this->headers_.clear();
    }

    bool empty() const
    {
      return this->url_.empty() && this->method_.empty();
    }

    const std::string & url() const {
      return this->url_;
    }

    const std::string & method() const {
      return this->method_;
    }
    const std::string & agent() const {
      return this->agent_;
    }

    const std::list<std::string> & headers() {
      return this->headers_;
    }

  private:
    std::string method_;
    std::string url_;
    std::string agent_;
    std::list<std::string> headers_;
  };
}

#endif // __VDS_HTTP_HTTP_REQUEST_H_
