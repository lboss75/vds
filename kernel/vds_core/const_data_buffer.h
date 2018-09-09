#ifndef __VDS_CORE_DATA_BUFFER_H_
#define __VDS_CORE_DATA_BUFFER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "targetver.h"
#include <vector>
#include <list>
#include "types.h"
#include "vds_debug.h"
#include <cstdlib>

namespace vds{
  class binary_serializer;
  class resizable_data_buffer;

  class const_data_buffer
  {
  public:
    const_data_buffer()
      : data_(nullptr), size_(0), allocated_size_(0)
    {
    }

    const_data_buffer(const void * data, size_t len)
      : data_(static_cast<uint8_t *>(len ? std::malloc(len) : nullptr)), size_(len), allocated_size_(len)
    {
      memcpy(this->data_, data, len);
    }
      
    const_data_buffer(const const_data_buffer & other)
      : data_(static_cast<uint8_t *>(other.size_ ? std::malloc(other.size_) : nullptr)), size_(other.size_), allocated_size_(other.size_)
    {
      memcpy(this->data_, other.data_, other.size_);
    }

    const_data_buffer(resizable_data_buffer && other);
      
    const_data_buffer(const_data_buffer&& other) noexcept
      : data_(other.data_), size_(other.size_), allocated_size_(other.allocated_size_)
    {
      other.data_ = nullptr;
      other.size_ = 0;
      other.allocated_size_ = 0;
    }
     

    ~const_data_buffer() {
      if (this->data_) {
        std::free(this->data_);
      }
    }
          
    const uint8_t * data() const { return this->data_; }
    uint8_t * data() { return this->data_; }
    size_t size() const { return this->size_; }
    
    void resize(size_t len) {
      if (this->allocated_size_ < len) {
        if (this->data_) {
          std::free(this->data_);
        }
        this->data_ = static_cast<uint8_t *>(malloc(len));
        this->allocated_size_ = len;
      }

      this->size_ = len;
    }

    const_data_buffer & operator = (const const_data_buffer & other)
    {
      this->resize(other.size_);
      memcpy(this->data_, other.data_, other.size_);

      return *this;
    }
    
    const_data_buffer & operator = (const_data_buffer && other) noexcept
    {
      if (this->data_) {
        std::free(this->data_);
      }
      this->data_ = other.data_;
      this->size_ = other.size_;
      this->allocated_size_ = other.allocated_size_;

      other.data_ = nullptr;
      other.size_ = 0;
      other.allocated_size_ = 0;

      return *this;
    }
     
    bool operator == (const const_data_buffer & other) const
    {
      return this->size_ == other.size_
      && 0 == memcmp(this->data_, other.data_, this->size_);
    }

    bool operator < (const const_data_buffer & other) const
    {
      return (this->size_ < other.size_)
      || ((this->size_ == other.size_) && (0 > memcmp(this->data_, other.data_, other.size_)));
    }

    bool operator > (const const_data_buffer & other) const
    {
      return (this->size_ > other.size_)
      || ((this->size_ == other.size_) && (0 < memcmp(this->data_, other.data_, this->size_)));
    }

    bool operator != (const const_data_buffer & other) const
    {
      return this->size_ != other.size_
        || 0 != memcmp(this->data_, other.data_, this->size_);
    }
    
    uint8_t operator[](size_t index) const
    {
      return this->data_[index];
    }

    uint8_t & operator[](size_t index)
    {
      return this->data_[index];
    }

    bool operator !() const
    {
      return this->size() == 0;
    }

    operator bool() const
    {
      return this->size() != 0;
    }

    void clear() {
      this->size_ = 0;
    }

  private:
    uint8_t * data_;
    size_t size_;
    size_t allocated_size_;
  };
}

#endif // __VDS_CORE_DATA_BUFFER_H_
