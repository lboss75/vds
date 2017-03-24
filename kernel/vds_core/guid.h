#ifndef __VDS_CORE_GUID_H_
#define __VDS_CORE_GUID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "data_buffer.h"

namespace vds {
  class guid : public data_buffer
  {
  public:
    guid();
    guid(const guid & other);
    guid(guid && other);
    
    static guid new_guid();

    std::string str() const;
    static guid parse(const std::string & value);

    guid & operator = (const guid & original);
    guid & operator = (guid && original);
    
  private:
    guid(const void * data, size_t len);
  };
}


#endif//__VDS_CORE_GUID_H_
