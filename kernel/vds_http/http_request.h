#ifndef __VDS_HTTP_HTTP_REQUEST_H_
#define __VDS_HTTP_HTTP_REQUEST_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include <list>
#include "http_message.h"

namespace vds {
  class http_request
  {
  public:
    http_request(const std::shared_ptr<http_message> & message);

    http_request(
      const std::string & method,
      const std::string & url,
      const std::string & agent = "HTTP/1.0"
      ): method_(method), url_(url), agent_(agent)
    {
      std::list<std::string> headers;
      headers.push_back(method + " " + url + " " + agent);

      this->message_ = std::make_shared<http_message>(headers, "");
      this->parse_parameters();
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
      return this->message_->headers();
    }

    bool get_header(const std::string & name, std::string & value) {
      return this->message_->get_header(name, value);
    }
    
    std::shared_ptr<http_message> get_message() const {
      return this->message_;
    }
    

  private:
    std::shared_ptr<http_message> message_;
    std::string method_;
    std::string url_;
    std::list<std::string> parameters_;
    std::string agent_;

    void parse_parameters();
  };
}

#endif // __VDS_HTTP_HTTP_REQUEST_H_
