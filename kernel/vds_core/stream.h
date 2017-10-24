#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <vector>
#include "async_task.h"

namespace vds {
  template <typename item_type>
  class stream_async
  {
  public:
    virtual ~stream_async() {}
    
    virtual async_task<> write_async(
      const service_provider & sp,
      const item_type * data,
      size_t len) = 0;
  };

  template <typename item_type>
  class stream
  {
  public:
	  virtual ~stream() {}

	  virtual void write(
      const service_provider & sp,
      const item_type * data,
      size_t len) = 0;
  };
  
  template <typename item_type>
  class collect_data : public stream<item_type>
  {
  public:
	  void write(
      const service_provider & sp,
      const item_type * data,
      size_t len) override
      {
        for(size_t i = 0; i < len; ++i){
          this->data_.push_back(data[i]);
        }
      }
      
    const item_type * data() const
    {
      return this->data_.data();
    }
    
    size_t size() const
    {
      return this->data_.size();
    }
      
  private:
    std::vector<item_type> data_;
  };
}

#endif//__VDS_CORE_STREAM_H_
