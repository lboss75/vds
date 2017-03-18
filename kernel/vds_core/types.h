#ifndef __VDS_CORE_TYPES_H_
#define __VDS_CORE_TYPES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#if defined(_WIN32)

#include <stdint.h>

typedef uint8_t u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;

#endif

#include <cstring>

namespace vds {
  class types {
  public:
    template <typename interface_type>
    static size_t get_type_id();

  private:
    static size_t g_last_type_id;
  };
}

template<typename interface_type>
inline size_t vds::types::get_type_id()
{
  static size_t type_id = ++g_last_type_id;
  return type_id;
}

#ifdef _WIN32

namespace vds {

  template <typename T>
  class _com_release
  {
  public:
    void operator()(T * p) {
      p->Release();
    }
  };

  template <typename T>
  using com_ptr = std::unique_ptr<T, _com_release<T>>;

  class _bstr_release
  {
  public:
    void operator()(BSTR p) {
      SysFreeString(p);
    }
  };

  using bstr_ptr = std::unique_ptr<OLECHAR, _bstr_release>;
}

#endif

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
    
    
  private:
    uint8_t * data_;
    size_t len_;
  };
}

#endif//__VDS_CORE_TYPES_H_
