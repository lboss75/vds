#ifndef __VDS_HTTP_HTTP_RESPONSE_H_
#define __VDS_HTTP_HTTP_RESPONSE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class http_request;
  
  class http_response : public std::enable_shared_from_this<http_response>
  {
  public:
    http_response(
      const std::function<void(
        const simple_done_handler_t & done,
        const error_handler_t & on_error,
        const std::shared_ptr<http_response> & response)> & complete_handler,
      const std::shared_ptr<http_request> & request
    );
    
    void set_result(int code, const std::string & comment) {
      this->stream_ << "HTTP/1.0 " << code << " " << comment << "\n";
    }
    
    void add_header(const std::string & name, const std::string & value) {
      this->stream_ << name << ":" << value << "\n";
    }
    
    void write_body(const std::string & value) {
      this->stream_ 
      << "Content-Length: " << value.length() << "\n"
      << "Connection: close\n\n"
      << value << "\n";
    }
    
    void complete(
      const simple_done_handler_t & done,
      const error_handler_t & on_error){
      this->complete_handler_(
        done,
        on_error,
        this->shared_from_this());
    }
    
    std::string result() const {
      return this->stream_.str();      
    }
    
  private:
    std::function<void(
      const simple_done_handler_t & done,
      const error_handler_t & on_error,
      const std::shared_ptr<http_response> & response)> complete_handler_;
    std::stringstream stream_;
  };
}

#endif // __VDS_HTTP_HTTP_RESPONSE_H_
