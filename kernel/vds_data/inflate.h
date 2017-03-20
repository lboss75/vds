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
    inflate(int compression_level);

    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const inflate & args
      ) : base_class(context), handler_(args.create_handler())
      {
      }

      void operator()(const void * data, size_t len) {
        const void * to_push;
        size_t to_push_len;
        if (!push_data(this->handler_, data, len, to_push, to_push_len)) {
          this->prev();
        }
        else {
          this->next(
            to_push,
            to_push_len);
        }
      }

      void processed()
      {
        const void * to_push;
        size_t to_push_len;
        if (!data_processed(this->handler_, to_push, to_push_len)) {
          this->prev();
        }
        else {
          this->next(
            to_push,
            to_push_len);
        }
      }

    private:
      _inflate_handler * handler_;
    };

  private:
    int compression_level_;

    _inflate_handler * create_handler() const;
    static void delete_handler(_inflate_handler * handler);
    static bool push_data(_inflate_handler * handler, const void * data, size_t size, const void *& to_push, size_t & to_push_len);
    static bool data_processed(_inflate_handler * handler, const void *& to_push, size_t & to_push_len);
  };
}

#endif // __VDS_DATA_INFLATE_H_
