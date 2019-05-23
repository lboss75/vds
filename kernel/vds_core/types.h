#ifndef __VDS_CORE_TYPES_H_
#define __VDS_CORE_TYPES_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "targetver.h"

#if defined(_WIN32)
#include <stdint.h>
#endif

#include <cstring>
#include <functional>
#include <memory>
#include <stdexcept>
#include "vds_debug.h"

namespace vds {
  class service_provider;

  class types {
  public:
    template <typename interface_type>
    static size_t get_type_id();

  private:
    static size_t g_last_type_id;
  };
  
  typedef std::function<void (const service_provider *, const std::exception_ptr &)> error_handler;
  
  enum class error_logic
  {
    throw_exception,
    return_false
  };

  template <typename target_type>
  class safe_cast
  {
  public:
    template <typename source_type>
    safe_cast(source_type value)
    : value_((target_type)value)
    {
      vds_assert(value == (source_type)this->value_);
    }

    template <typename source_type>
    safe_cast(source_type value, target_type mask)
        : value_(mask & (target_type)value)
    {
      vds_assert(value == (source_type)this->value_);
    }

    operator target_type () const {
      return this->value_;
    }

  private:
    target_type value_;
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

#endif//__VDS_CORE_TYPES_H_
