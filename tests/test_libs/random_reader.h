#ifndef RANDOM_READER_H
#define RANDOM_READER_H

#include "targetver.h"
#include "types.h"
#include "dataflow.h"

template<typename item_type>
class random_reader
{
public:
  random_reader(
    const item_type * data,
    size_t len)
  : data_(data),
    len_(len)
  {
  }

  using outgoing_item_type = item_type;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_source<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_source<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const random_reader & args)
      : base_class(context),
      data_(args.data_),
      len_(args.len_)
    {
    }

    size_t sync_get_data(
      const vds::service_provider & sp)
    {
      for (;;) {
        size_t n = (size_t)std::rand();
        if (n < 1 || n > this->output_buffer_size_) {
          continue;
        }

        if (n > this->len_) {
          n = this->len_;
        }
        
        if(0 == n){
          return 0;
        }

        memcpy(this->output_buffer_, this->data_, n);

        this->data_ += n;
        this->len_ -= n;

        return n;
      }
    }

  private:
    const item_type * data_;
    size_t len_;
  };

private:
  const item_type * data_;
  size_t len_;
};

#endif // RANDOM_READER_H
