#ifndef __VDS_HTTP_HTTP_STREAM_READER_H_
#define __VDS_HTTP_HTTP_STREAM_READER_H_

namespace vds {
  template <
    typename next_method_type,
    typename error_method_type
  >
  class http_stream_reader
  {
  public:
    http_stream_reader()
    {

    }

    bool read_async()
    {
      return false;
    }

  private:
    next_method_type * next_method_;
    error_method_type * error_method_;
  };
}

#endif//__VDS_HTTP_HTTP_STREAM_READER_H_
