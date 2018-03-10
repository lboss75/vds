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

    static http_message simple_text_response(
      const service_provider & sp,
      const std::string & body,
      int result_code = HTTP_OK,
      const std::string & message = "OK");

    static http_message status_response(
        const service_provider & sp,
        int result_code,
        const std::string & message);

  private:
    std::string protocol_;
    int code_;
    std::string comment_;
    std::list<std::string> headers_;

    http_message create_message(const service_provider & sp) const;
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_H_
