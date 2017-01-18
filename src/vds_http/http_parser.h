#ifndef __VDS_HTTP_HTTP_PARSER_H
#define __VDS_HTTP_HTTP_PARSER_H

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <string>
#include <list>
#include <string.h>

#include "func_utils.h"
#include "service_provider.h"
#include "http_request.h"

namespace vds {
  class http_router;
  class http_parser
  {
  public:
    http_parser();
    

    template<
      typename done_method_type,
      typename next_method_type,
      typename error_method_type
    >
    class handler
    {
    public:
      handler(
        done_method_type & done_method,
        next_method_type & next_method,
        error_method_type & error_method,
        const http_parser & args){
      }
    
      
      template<typename next_filter>
      void operator()(
        const std::function<void(void)> & done,
        const error_handler_t & on_error,
        next_filter next,
        const char * data,
        size_t len
      ) {
        this->process<next_filter>(done, on_error, next, data, len);
      }
      
      void processed(http_request_body_handler * body_handler)
      {

      }

    private:
      std::string parse_buffer_;
      std::list<std::string> headers_;
      std::shared_ptr<http_request> request_;
      
      template<typename next_filter>
      void process(
        const std::function<void(void)> & done,
        const vds::error_handler_t & on_error,
        next_filter next,
        const char * data,
        size_t len) {
        
        if(0 == len) {
          //Close connection
          return;
        }
        
        //request body
        if(this->request_){
          this->request_->push_data(
            [this, done, on_error, next, data, len](size_t readed, bool continue_read) {
              if(!continue_read){
                this->request_.reset();
              }
              auto p = data + readed;
              auto l = len - readed;
              if(0 == l) {
                done();
              } else {
                this->process<next_filter>(
                  done,
                  on_error,
                  next,
                  p,
                  l
                );
              }
            },
            on_error,
            data,
            len);
        }
        
        while (len > 0) {
          const char * p = (const char *)memchr(data, '\n', len);
          if (nullptr == p) {
            this->parse_buffer_ += std::string(data, len);
            break;
          }
          
          auto size = p - data;
          
          if(size > 0) {
            if ('\r' == data[size - 1]) {
              this->parse_buffer_ += std::string(data, size - 1);
            } else {
              this->parse_buffer_ += std::string(data, size);
            }
          }
          
          if (0 == this->parse_buffer_.length()) {
            if (this->headers_.empty()) {
              throw new std::logic_error("Invalid request");
            }
            
            auto request = *this->headers_.begin();

            std::string items[3];

            size_t index = 0;
            for (auto ch : request) {
              if (isspace(ch)) {
                ++index;
                if (index > sizeof(items) / sizeof(items[0])) {
                  throw new std::logic_error("Invalid request");
                }
              }
              else {
                items[index] += ch;
              }
            }

            if ((index + 1) != sizeof(items) / sizeof(items[0])) {
              throw new std::logic_error("Invalid request");
            }

            this->headers_.pop_front();
            
            this->request_.reset(
              new http_request(
                items[0],
                items[1],
                items[2],
                this->headers_,
                done,
                p + 1,
                len - size - 1));
            this->headers_.clear();
            
            next(
              [
                this,
                done,
                on_error,
                next,
                data = p + 1, len = len - size - 1
              ](){
                  if(len > 0) {
                    this->process<next_filter>(
                      done,
                      on_error,
                      next,
                      data,
                      len
                    );
                  }
                  else {
                    done();
                  }
              },
              on_error,
              this->request_
            );
            
            return;
          } else {
            this->headers_.push_back(this->parse_buffer_);
            this->parse_buffer_.clear();
          }
          
          len -= size + 1;
          data = p + 1;    
        }
        
        done();        
      }
    };
  };
}

#endif // HTTP_PARSER_H
