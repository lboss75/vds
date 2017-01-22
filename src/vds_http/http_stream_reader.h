#ifndef __VDS_HTTP_HTTP_STREAM_READER_H_
#define __VDS_HTTP_HTTP_STREAM_READER_H_

#include <vector>
#include "http_incoming_stream.h"

namespace vds {
  class http_stream_reader
  {
  public:
    http_stream_reader(
      http_incoming_stream & incoming_stream
    )
    : incoming_stream_(incoming_stream)
    {
    }
    
    template<
      typename next_method_type,
      typename error_method_type
    >
    class handler : public http_incoming_stream::read_handler
    {
    public:
      handler(
        next_method_type & next_method,
        error_method_type & error_method,
        const http_stream_reader & args
      )
      : next_method_(next_method),
        error_method_(error_method),
        incoming_stream_(args.incoming_stream_),
        wait_handler_(false)
      {
        this->incoming_stream_.handler(this);
      }
      
      void operator()()
      {
        this->data_mutex_.lock();
        
        if(this->data_.empty()){
          this->wait_handler_ = true;
          this->data_mutex_.unlock();
        }
        else {  
          std::pair<const void *, size_t> t = *this->data_.rbegin();
          this->data_.pop_front();
          this->data_mutex_.unlock();
          
          this->next_method_(t.first, t.second);
        }
      }
      
      void push_data(
        const void * data,
        size_t len
      ){
        this->data_mutex_.lock();;
        
        this->data_.push_back(
          std::pair<const void *, size_t>(
            data, len));
        
        if(this->wait_handler_) {
          this->wait_handler_ = false;
          std::pair<const void *, size_t> t = *this->data_.rbegin();
          this->data_.pop_front();
          this->data_mutex_.unlock();
          
          this->next_method_(t.first, t.second);
        }
        else {
          this->data_mutex_.unlock();
        }
      }
      
    private:
      next_method_type & next_method_;
      error_method_type & error_method_;
      http_incoming_stream & incoming_stream_;
      
      std::mutex data_mutex_;
      bool wait_handler_;
      std::list<std::pair<const void *, size_t>> data_;
    };
  private:
    http_incoming_stream & incoming_stream_;
  };
}

#endif//__VDS_HTTP_HTTP_STREAM_READER_H_
