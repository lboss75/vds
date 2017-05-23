#ifndef __VDS_HTTP_HTTP_RESPONSE_H_
#define __VDS_HTTP_HTTP_RESPONSE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_message.h"

namespace vds {
  class http_response
  {
  public:
    static constexpr int HTTP_OK = 200;
    static constexpr int HTTP_Internal_Server_Error = 500;
    static constexpr int HTTP_Not_Found = 404;

    http_response(
      int code,
      const std::string & comment);

    void add_header(const std::string & name, const std::string & value) {
      this->headers_.push_back(name + ":" + value);
    }
    
    int code() const
    {
      return this->code_;
    }

    const std::string & comment() const
    {
      return this->comment_;
    }

    std::shared_ptr<http_message> create_message() const;

  private:
    std::string protocol_;
    int code_;
    std::string comment_;

    std::list<std::string> headers_;
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_H_
