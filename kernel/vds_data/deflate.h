#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "dataflow.h"

namespace vds {
  class _deflate;
  class _deflate_handler;

  //Compress data
  class deflate
  {
  public:
    deflate();
    deflate(int compression_level);

    using incoming_item_type = uint8_t;
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 10;

    template <typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_filter<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const deflate & args
      ) : base_class(context), handler_(args.create_handler())
      {
      }

      ~handler()
      {
        delete_handler(this->handler_);
      }

      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        update_data(
          this->handler_,
          this->input_buffer(),
          this->input_buffer_size(),
          this->output_buffer(),
          this->output_buffer_size(),
          input_readed,
          output_written);
      }
      
    private:
      _deflate_handler * handler_;
    };

  private:
    int compression_level_;

    _deflate_handler * create_handler() const;
    static void delete_handler(_deflate_handler * handler);
    static void update_data(
      _deflate_handler * handler,
      const void * input_data,
      size_t input_size,
      void * output_data,
      size_t output_size,
      size_t & readed,
      size_t & written);
  };
}

#endif // __VDS_DATA_DEFLATE_H_
