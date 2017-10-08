#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _deflate_handler;

  //Compress data
  class deflate
  {
  public:
    deflate();
    deflate(int compression_level);
    ~deflate();

    void update_data(
      const void * input_data,
      size_t input_size,
      void * output_data,
      size_t output_size,
      size_t & readed,
      size_t & written);

  private:
      _deflate_handler * const impl_;
  };
}

#endif // __VDS_DATA_DEFLATE_H_
