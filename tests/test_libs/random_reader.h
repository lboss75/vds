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
        size_t n = (size_t)std::rand() % this->output_buffer_size();
        if (n < 1 || n > this->output_buffer_size()) {
          continue;
        }

        if (n > this->len_) {
          n = this->len_;
        }
        
        if(0 == n){
          return 0;
        }

        std::copy(this->data_, this->data_ + n, this->output_buffer());

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


template<typename item_type>
class random_filter
{
public:
  random_filter()
  {
  }

  using incoming_item_type = item_type;
  using outgoing_item_type = item_type;
  static constexpr size_t BUFFER_SIZE = 532;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_filter<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const random_filter & args)
      : base_class(context)
    {
    }

    void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
    {
      auto n = (this->input_buffer_size() < this->output_buffer_size())
        ? this->input_buffer_size()
        : this->output_buffer_size();

      if(0 != n) {
        size_t l = (size_t)std::rand() % n;

        std::copy(this->input_buffer(), this->input_buffer() + l, this->output_buffer());

        input_readed = l;
        output_written = l;
      }
      else {
        input_readed = 0;
        output_written = 0;
      }
    }
  };
};

#endif // RANDOM_READER_H
