/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#ifndef __TEST_VDS_LIBS__RANDOM_FILTER_H_
#define __TEST_VDS_LIBS__RANDOM_FILTER_H_

#include "targetver.h"
#include "types.h"
#include "dataflow.h"

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
        size_t l = ((size_t)std::rand() % n) + 1;
        

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

#endif // __TEST_VDS_LIBS__RANDOM_FILTER_H_
