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

#endif//__VDS_CORE_TYPES_H_
