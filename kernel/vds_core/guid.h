#ifndef __VDS_CORE_GUID_H_
#define __VDS_CORE_GUID_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "types.h"

namespace vds {
  class guid : public data_buffer
  {
  public:
    guid();
    guid(const guid & other);
    guid(guid && other);
    
    static guid new_guid();

    std::string str() const;
    
  private:
    guid(const void * data, size_t len);
  };
}


#endif//__VDS_CORE_GUID_H_
