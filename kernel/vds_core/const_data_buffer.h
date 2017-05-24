#ifndef __VDS_CORE_DATA_BUFFER_H_
#define __VDS_CORE_DATA_BUFFER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <vector>
#include "dataflow.h"

namespace vds{
  class binary_serializer;

  class const_data_buffer
  {
  public:
    const_data_buffer()
    : data_(nullptr), len_(0)
    {
    }
    
    const_data_buffer(const void * data, size_t len)
    : data_(new uint8_t[len]), len_(len)
    {
      memcpy(this->data_, data, len);
    }
    
    const_data_buffer(const const_data_buffer & other)
    : data_(new uint8_t[other.len_]), len_(other.len_)
    {
      memcpy(this->data_, other.data_, other.len_);
    }
    
    const_data_buffer(const_data_buffer&& other)
    : data_(other.data_), len_(other.len_)
    {
      other.data_ = nullptr;
      other.len_ = 0;
    }
    
    const_data_buffer(const std::vector<uint8_t> & data)
    : data_(new uint8_t[data.size()]), len_(data.size())
    {
      memcpy(this->data_, data.data(), data.size());
    }
    
    ~const_data_buffer()
    {
      delete this->data_;
    }
    
    const uint8_t * data() const { return this->data_; }
    size_t size() const { return this->len_; }
    
    void reset(const void * data, size_t len)
    {
      delete this->data_;
      this->data_ = new uint8_t[len];
      this->len_ = len;
      memcpy(this->data_, data, len);
    }

    const_data_buffer & operator = (const_data_buffer && other)
    {
      delete this->data_;
      this->data_ = other.data_;
      this->len_ = other.len_;
      other.data_ = nullptr;
      other.len_ = 0;
      
      return *this;
    }
    
    const_data_buffer & operator = (const const_data_buffer & other)
    {
      this->reset(other.data(), other.size());
      
      return *this;
    }    
    
    bool operator == (const const_data_buffer & other) const
    {
      return this->len_ == other.len_
      && 0 == memcmp(this->data_, other.data_, this->len_);
    }
    
    bool operator != (const const_data_buffer & other) const
    {
      return this->len_ != other.len_
      || 0 != memcmp(this->data_, other.data_, this->len_);
    }
    
    uint8_t operator[](size_t index) const
    {
      return this->data_[index];
    }

    void serialize(binary_serializer & s) const;
    
  private:
    uint8_t * data_;
    size_t len_;
  };

  class collect_data
  {
  public:
    collect_data(std::vector<uint8_t> & buffer)
    : buffer_(buffer)
    {
    }
    
    using incoming_item_type = uint8_t;
    using outgoint_item_type = uint8_t;
    static const size_t BUFFER_SIZE = 1024;
    static const size_t MIN_BUFFER_SIZE = 1;

    template <typename context_type>
    class handler : public sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;

    public:
      handler(
        const context_type & context,
        const collect_data & args)
        : base_class(context),
          buffer_(args.buffer_)
      {
      }

      size_t sync_push_data(const service_provider & sp)
      {
        this->buffer_.insert(this->buffer_.end(), this->input_buffer_, this->input_buffer_ + this->input_buffer_size_);
        return this->input_buffer_size_;
      }

    private:
      std::vector<uint8_t> & buffer_;
    };
  private:
    std::vector<uint8_t> & buffer_;
  };
}

#endif // __VDS_CORE_DATA_BUFFER_H_
