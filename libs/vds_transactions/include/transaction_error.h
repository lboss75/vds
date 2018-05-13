#ifndef __VDS_TRANSACTION_TRANSACTION_ERROR_H_
#define __VDS_TRANSACTION_TRANSACTION_ERROR_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdexcept>

namespace vds {
  class transaction_error : public std::runtime_error {
  public:
    transaction_error(const std::string & message)
        : std::runtime_error(message){

    }
  };
}

#endif //__VDS_TRANSACTION_TRANSACTION_ERROR_H_
