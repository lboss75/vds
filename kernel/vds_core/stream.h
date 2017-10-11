#ifndef __VDS_CORE_STREAM_H_
#define __VDS_CORE_STREAM_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  template <typename item_type>
  class stream
  {
  public:
    virtual ~stream() {}
    
    virtual void write(const item_type * data, size_t len) = 0;
    virtual void final() = 0;
  };

}

#endif//__VDS_CORE_STREAM_H_
