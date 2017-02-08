#ifndef __VDS_STORAGE_STORAGE_CURSOR_H_
#define __VDS_STORAGE_STORAGE_CURSOR_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {

  template <typename data_type>
  class storage_cursor
  {
  public:
    bool read();

    data_type & current();
  };
}

#endif // __VDS_STORAGE_STORAGE_CURSOR_H_
