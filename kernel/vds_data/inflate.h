#ifndef __VDS_DATA_INFLATE_H_
#define __VDS_DATA_INFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "const_data_buffer.h"

namespace vds {
  class _inflate_handler;

  //Decompress stream
  class inflate
  {
  public:
    inflate();
    ~inflate();

    void update_data(
      const void * input_data,
      size_t input_size,
      
      void * output_data,
      size_t output_size,
      
      size_t & readed,
      size_t & written);

  private:
    _inflate_handler * const impl_;
  };
}

#endif // __VDS_DATA_INFLATE_H_
