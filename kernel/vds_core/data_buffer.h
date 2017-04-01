#ifndef __VDS_CORE_DATA_BUFFER_H_
#define __VDS_CORE_DATA_BUFFER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "dataflow.h"

namespace vds{
  class data_buffer
  {
  public:
    data_buffer()
    : data_(nullptr), len_(0)
    {
    }
    
    data_buffer(const void * data, size_t len)
    : data_(new uint8_t[len]), len_(len)
    {
      memcpy(this->data_, data, len);
    }
    
    data_buffer(const data_buffer & other)
    : data_(new uint8_t[other.len_]), len_(other.len_)
    {
      memcpy(this->data_, other.data_, other.len_);
    }
    
    data_buffer(data_buffer&& other)
    : data_(other.data_), len_(other.len_)
    {
      other.data_ = nullptr;
      other.len_ = 0;
    }
    
    data_buffer(const std::vector<uint8_t> & data)
    : data_(new uint8_t[data.size()]), len_(data.size())
    {
      memcpy(this->data_, data.data(), data.size());
    }
    
    ~data_buffer()
    {
      delete this->data_;
    }
    
    const uint8_t * data() const { return this->data_; }
    size_t size() const { return this->len_; }
    
    void reset(const void * data, size_t len)
    {
      this->resize(len);
      memcpy(this->data_, data, len);
    }

    void resize(size_t len)
    {
      delete this->data_;
      this->data_ = new uint8_t[len];
      this->len_ = len;
    }
    
    data_buffer & operator = (data_buffer && other)
    {
      delete this->data_;
      this->data_ = other.data_;
      this->len_ = other.len_;
      other.data_ = nullptr;
      other.len_ = 0;
      
      return *this;
    }
    
    data_buffer & operator = (const data_buffer & other)
    {
      this->reset(other.data(), other.size());
      
      return *this;
    }    
    
    bool operator == (const data_buffer & other) const
    {
      return this->len_ == other.len_
      && 0 == memcmp(this->data_, other.data_, this->len_);
    }
    
    bool operator != (const data_buffer & other) const
    {
      return this->len_ != other.len_
      || 0 != memcmp(this->data_, other.data_, this->len_);
    }
    
    uint8_t & operator[](size_t index)
    {
      return this->data_[index];
    }
    
    uint8_t operator[](size_t index) const
    {
      return this->data_[index];
    }
    
    void add(const void * data, size_t size)
    {
      uint8_t * new_data = new uint8_t[this->len_ + size];
      if(0 < this->len_){
        memcpy(new_data, this->data_, this->len_);
      }
      memcpy(new_data + this->len_, data, size);
      this->len_ += size;
      delete this->data_;
      this->data_ = new_data;
    }
    
  private:
    uint8_t * data_;
    size_t len_;
  };

  class collect_data
  {
  public:
    collect_data()
    {
    }

    template <typename context_type>
    class handler : public dataflow_step<context_type, void(const void *, size_t)>
    {
      using base_class = dataflow_step<context_type, void(const void *, size_t)>;

    public:
      handler(
        const context_type & context,
        const collect_data & args)
        : base_class(context)
      {
      }

      void operator ()(const void * data, size_t len)
      {
        if (0 == len) {
          this->next(this->target_.data(), this->target_.size());
        }
        else {
          this->target_.add(data, len);
          this->prev();
        }
      }

    private:
      data_buffer target_;
    };
  };
}

#endif // __VDS_CORE_DATA_BUFFER_H_
