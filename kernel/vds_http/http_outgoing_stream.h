#ifndef __VDS_HTTP_HTTP_OUTGOING_STREAM_H_
#define __VDS_HTTP_HTTP_OUTGOING_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "http_response.h"
#include "http_stream_reader.h"

namespace vds {
  
  class http_outgoing_stream
  {
  public:
    http_outgoing_stream();
    
    void set_body(const std::string & body)
    {
      this->body_ = body;
      this->is_simple_ = true;
    }

    void set_file(const filename & file)
    {
      this->file_ = file;
      this->is_simple_ = false;
    }

    bool is_simple() const
    {
      return this->is_simple_;
    }

    const std::string & body() const {
      return this->body_;
    }
    
    size_t size() const {
      return this->is_simple_ ? this->body_.size() : file::length(this->file_);
    }

    const filename & file() const {
      return this->file_;
    }

    void clear()
    {
      this->is_simple_ = true;
      this->body_.clear();
    }

  private:
    bool is_simple_;
    std::string body_;
    filename file_;
  };
}

#endif // __HTTP_RESPONSE_STREAM_H_
