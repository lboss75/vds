#ifndef __VDS_DATA_INFLATE_H_
#define __VDS_DATA_INFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "const_data_buffer.h"
#include "stream.h"

namespace vds {
  class _inflate_handler;

  //Decompress stream
  class inflate : public stream_output_async<uint8_t>
  {
  public:
    inflate();
    ~inflate();

    expected<void> create(
      const std::shared_ptr<stream_output_async<uint8_t>> & target);

	  static expected<const_data_buffer> decompress(
      const void * data,
      size_t size);

    vds::async_task<vds::expected<void>> write_async(        
        const uint8_t * data,
        size_t len) override ;

  private:
    _inflate_handler * impl_;
  };

}

#endif // __VDS_DATA_INFLATE_H_
