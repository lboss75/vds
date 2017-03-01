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
      const std::string & addresses
    ) : addresses_(addresses)
    {
    }

    const std::string & addresses() const {
      return this->addresses_;
    }

  private:
    std::string addresses_;
  };

  template <>
  class storage_cursor<endpoint> : public _simple_storage_cursor<endpoint>
  {
  public:
    storage_cursor(const istorage & storage);
  };  
}

#endif // __VDS_STORAGE_ENDPOINT_H_
