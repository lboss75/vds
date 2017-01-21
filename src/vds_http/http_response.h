#ifndef __VDS_HTTP_HTTP_RESPONSE_H_
#define __VDS_HTTP_HTTP_RESPONSE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_request;
  
  class http_response
  {
  public:
    static constexpr int HTTP_OK = 200;
    
    void reset(
      const http_request & request
    );
    
    void reset(
      const std::string & protocol,
      const std::string & code,
      const std::string & comment,
      const std::list<std::string> & headers 
    )
    {
      this->protocol_ = protocol;
      this->code_ = atoi(code.c_str());
      this->comment_ = comment;
      this->headers_ = headers;
    }
    
    void set_result(int code, const std::string & comment) {
      this->code_ = code;
      this->comment_ = comment;
    }
    
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

    const std::list<std::string> headers() const
    {
      return this->headers_;
    }

  private:
    std::string protocol_;
    int code_;
    std::string comment_;
    std::list<std::string> headers_;
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_H_
