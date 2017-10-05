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
        const inflate & args
      ) : base_class(context), handler_(args.create_handler())
      {
      }

      void sync_process_data(
        const vds::service_provider & sp,
        size_t & input_readed,
        size_t & output_written)
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
      _inflate_handler * handler_;
    };
    
    //static const_data_buffer inflate_buffer(const const_data_buffer & data);

  private:
    _inflate_handler * create_handler() const;
    static void delete_handler(_inflate_handler * handler);
    static void update_data(
      _inflate_handler * handler,
      const void * input_data,
      size_t input_size,
      void * output_data,
      size_t output_size,
      size_t & readed,
      size_t & written);
  };
}

#endif // __VDS_DATA_INFLATE_H_
