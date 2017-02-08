#ifndef __VDS_STORAGE_ENDPOINT_H_
#define __VDS_STORAGE_ENDPOINT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_cursor.h"

namespace vds {
  class istorage;

  class endpoint
  {
  public:
    endpoint(
      const std::string & address,
      int port
    ) : address_(address), port_(port)
    {
    }

    const std::string & address() const {
      return this->address_;
    }

    int port() const {
      return this->port_;
    }

  private:
    std::string address_;
    int port_;
  };

  template <>
  class storage_cursor<endpoint>
  {
  public:
    storage_cursor(istorage & storage);

    bool read();

    endpoint & current();

  private:
    size_t index_;
  };  
}

#endif // __VDS_STORAGE_ENDPOINT_H_
