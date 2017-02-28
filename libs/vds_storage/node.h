#ifndef __VDS_STORAGE_NODE_H_
#define __VDS_STORAGE_NODE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "storage_cursor.h"

namespace vds {
  class istorage;

  class node
  {
  public:

    const std::string & id() const { return this->id_; }

  private:
    std::string id_;
  };

  template <>
  class storage_cursor<node> : public _simple_storage_cursor<node>
  {
  public:
    storage_cursor(istorage & storage);
  };

}

#endif // __VDS_STORAGE_NODE_H_
