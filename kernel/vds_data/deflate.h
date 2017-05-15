#ifndef __VDS_DATA_DEFLATE_H_
#define __VDS_DATA_DEFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

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

      bool sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        const void * to_push;
        size_t to_push_len;
        if (!push_data(this->handler_, data, len, to_push, to_push_len)) {
          return true;
        }
        else {
          if(this->next(
            sp,
            to_push,
            to_push_len)){
              for(;;){
                const void * to_push;
                size_t to_push_len;
                if (!data_processed(this->handler_, to_push, to_push_len)) {
                  return true;
                }
                else {
                  if(!this->next(
                    sp,
                    to_push,
                    to_push_len)){
                    return false;
                  }
                }
              }
          }
          
          return false;
        }
      }

    private:
      _deflate_handler * handler_;
    };

  private:
    int compression_level_;

    _deflate_handler * create_handler() const;
    static void delete_handler(_deflate_handler * handler);
    static bool push_data(_deflate_handler * handler, const void * data, size_t size, const void *& to_push, size_t & to_push_len);
    static bool data_processed(_deflate_handler * handler, const void *& to_push, size_t & to_push_len);
  };
}

#endif // __VDS_DATA_DEFLATE_H_
