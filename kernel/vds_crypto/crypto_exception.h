#ifndef __VDS_CRYPTO_CRYPTO_EXCEPTION_H_
#define __VDS_CRYPTO_CRYPTO_EXCEPTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <exception>

namespace vds {
  
  class crypto_exception : public std::runtime_error
  {
  public:
    crypto_exception(const std::string & message, int error_code);

  };

}

#endif // __CRYPTO_EXCEPTION_H_
