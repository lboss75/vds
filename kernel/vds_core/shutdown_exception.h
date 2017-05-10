#ifndef __VDS_CORE_SHUTDOWN_EXCEPTION_H_
#define __VDS_CORE_SHUTDOWN_EXCEPTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include <exception>
#include <stdexcept>

namespace vds {
  class shutdown_exception : public std::runtime_error
  {
  public:
    shutdown_exception();

  };
}

#endif // __VDS_CORE_SHUTDOWN_EXCEPTION_H_
