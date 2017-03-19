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
    
    file_container& add(const std::string & name, const std::string & body)
    {
      this->items_.push_back(item {item::it_string, name, body });
      
      return *this;
    }
    
  private:
    struct item
    {
    public:
      enum item_type
      {
        it_string,
        it_text_file,
        it_binary_file
      };
      
      item_type type;
      std::string name;
      std::string string_body;
      filename file_name;
    };
    
    std::list<item> items_;
  };
}

#endif // __VDS_DATA_FILE_CONTAINER_H_
