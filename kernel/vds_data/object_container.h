#ifndef __VDS_DATA_OBJECT_CONTAINER_H_
#define __VDS_DATA_OBJECT_CONTAINER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <unordered_map>

#include "binary_serialize.h"

namespace vds {
  class object_container
  {
  public:
    object_container();
    object_container(binary_deserializer & s);
    object_container(binary_deserializer && s);

    object_container& add(const std::string & name, const std::string & body);

    binary_serializer & serialize(binary_serializer & s) const;

    std::string get(const std::string & name) const;

  private:
    std::unordered_map<std::string, std::string> items_;
  };
}

#endif // __VDS_DATA_OBJECT_CONTAINER_H_
