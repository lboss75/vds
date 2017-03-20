#ifndef __VDS_DATA_DEFLATE_P_H_
#define __VDS_DATA_DEFLATE_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _deflate_handler
  {
  public:
    _deflate_handler(int compression_level);

    bool push_data(const void * data, size_t len, const void *& next_data, size_t & next_len);
    bool processed(const void *& next_data, size_t & next_len);

  private:
    static constexpr size_t CHUNK_SIZE = 1024;
    unsigned char buffer_[CHUNK_SIZE];
    z_stream strm_;
    bool eof_;
  };
}

#endif // __VDS_DATA_DEFLATE_P_H_
