#ifndef __VDS_DATA_FILE_CONTAINER_H_
#define __VDS_DATA_FILE_CONTAINER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class file_container
  {
  public:
    file_container& add(const std::string & name, const std::string & body);

    binary_serializer & serialize(binary_serializer & s) const;
    
  private:
    struct item
    {
    public:
      std::string name;
      std::string body;
    };
    
    std::list<item> items_;
  };
}

#endif // __VDS_DATA_FILE_CONTAINER_H_
