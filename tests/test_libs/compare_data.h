#ifndef COMPARE_DATA_H
#define COMPARE_DATA_H

#include "targetver.h"
#include "dataflow.h"

template <typename item_type>
class compare_data
{
public:
  compare_data(
    const item_type * data,
    size_t len)
    : data_(data),
    len_(len)
  {
  }

  using incoming_item_type = item_type;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_target<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_target<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const compare_data & args)
      : base_class(context),
      data_(args.data_),
      len_(args.len_),
      in_error_(false)

    {
    }

    ~handler()
    {
      if (0 != this->len_ && !this->in_error_) {
        throw std::runtime_error("Unexpected end of stream while comparing data");
      }
    }

    size_t sync_push_data(
      const vds::service_provider & sp)
    {
      if (0 == this->input_buffer_size()) {
        if (0 != this->len_) {
          this->in_error_ = true;
          this->error(sp, std::make_shared<std::runtime_error>("Unexpected end of stream while comparing data"));
        }
        return 0;
      }

      if (this->len_ < this->input_buffer_size()) {
        this->in_error_ = true;
        this->error(sp, std::make_shared<std::runtime_error>("Unexpected data while comparing data"));
        return 0;
      }

      if (0 != memcmp(this->data_, this->input_buffer(), this->input_buffer_size())) {
        this->in_error_ = true;
        this->error(sp, std::make_shared<std::runtime_error>("Compare data error"));
        return 0;
      }

      this->data_ += this->input_buffer_size();
      this->len_ -= this->input_buffer_size();

      return this->input_buffer_size();
    }

  private:
    const item_type * data_;
    size_t len_;
    bool in_error_;
  };
private:
  const item_type * data_;
  size_t len_;
};

#endif // COMPARE_DATA_H
