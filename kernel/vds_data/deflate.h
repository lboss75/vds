#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stream.h"

namespace vds {
  class _deflate_handler;
  class const_data_buffer;

  //Compress data
  class deflate : public stream_output_async<uint8_t>
  {
  public:
    deflate();
    deflate(_deflate_handler * impl)
      : impl_(impl) {      
    }

    ~deflate();

    static expected<std::shared_ptr<deflate>> create(const std::shared_ptr<stream_output_async<uint8_t>> & target);
    static expected<std::shared_ptr<deflate>> create(const std::shared_ptr<stream_output_async<uint8_t>> & target, int compression_level);

    static expected<const_data_buffer> compress(
      const uint8_t * data,
      size_t len);
    
    static expected<const_data_buffer> compress(      
      const const_data_buffer & data);

    vds::async_task<vds::expected<void>> write_async(        
        const uint8_t *data,
        size_t len) override;

  private:
    _deflate_handler * impl_;
  };

}

#endif // __VDS_DATA_DEFLATE_H_
