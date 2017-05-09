#ifndef __VDS_CORE_DATA_PIPELINE_H_
#define __VDS_CORE_DATA_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "circular_buffer.h"

namespace vds{
  
  template <typename source_type, typename target_type>
  class data_pipeline
  {
  public:
    
  };
  
  template <typename source_type, typename target_type>
  class stream_pipeline
  {
  public:
    
    void start()
    {
      this->source_.query_data(this->buffer_, this->buffer_size_);
    }
    
    bool operator()(const void * data, size_t data_size)
    {
      return true;
      
      this->prev();
    }
    
    class incomming_pipeline
    {
    public:
      
      void start()
      {
        source_
          .query_data(this->buffer_, this->buffer_size_)
          .wait();
      }
      
      void operator()(size_t size)
      {
        
      }
      
    private:
      source_type & source_;
    };
    
  private:
    source_type & source_;
  };
  
  
  class deflate
  {
  public:
    
    void push_data(const void * data, size_t size)
    {
      this->next();
    }
    
    class handler
    {
    public:
      size_t query_data(void * data, size_t size)
      {
      }
      
      bool operator()(const void * data, size_t size)
      {
        if(this->next()) {
        }
      }
      
      void processed()
      {
      }
    };
  };
  
}

#endif // __VDS_CORE_DATA_PIPELINE_H_
