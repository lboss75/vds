#ifndef __VDS_DATA_INFLATE_H_
#define __VDS_DATA_INFLATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


namespace vds {
  class _inflate_handler;

  //Decompress stream
  class inflate
  {
  public:
    inflate();

    template <typename context_type>
    class handler : public dataflow_step<context_type, bool(const void *, size_t)>
    {
      using base_class = dataflow_step<context_type, bool(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const inflate & args
      ) : base_class(context), handler_(args.create_handler())
      {
      }

      bool operator()(const service_provider & sp, const void * data, size_t len) {
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
            return true;
          }
          
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
      }

      void processed(const service_provider & sp)
      {
        for(;;){
          const void * to_push;
          size_t to_push_len;
          if (!data_processed(this->handler_, to_push, to_push_len)) {
            this->prev(sp);
            return;
          }
          else {
            if(!this->next(
              sp,
              to_push,
              to_push_len)){
              return;
            }
          }
        }
      }

    private:
      _inflate_handler * handler_;
    };
    
    static const_data_buffer inflate_buffer(const const_data_buffer & data);

  private:
    _inflate_handler * create_handler() const;
    static void delete_handler(_inflate_handler * handler);
    static bool push_data(_inflate_handler * handler, const void * data, size_t size, const void *& to_push, size_t & to_push_len);
    static bool data_processed(_inflate_handler * handler, const void *& to_push, size_t & to_push_len);
  };
}

#endif // __VDS_DATA_INFLATE_H_
