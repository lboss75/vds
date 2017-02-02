#ifndef __VDS_CORE_FILENAME_H_
#define __VDS_CORE_FILENAME_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class foldername;
  
  class filename
  {
  public:
      filename()
      {
      }
      
      filename(const filename& other)
      : value_(other.value_)
      {
      }
      
      filename(
        const foldername & base,
        const std::string & relative);
      
      
      filename(const std::string & filepath)
      : value_(filepath)
      {
      }
      
      filename & operator=(const filename& other)
      {
        this->value_ = other.value_;
        return *this;
      }
      
      bool operator== (const filename& other) const
      {
        return this->value_ == other.value_;
      }
      
      std::string name() const;
      std::string name_without_extension() const;
      std::string extension() const;

      const char * c_str() const
      {
        return this->value_.c_str();
      }
      
  private:
    std::string value_;
  };
}

#endif // __VDS_CORE_FILENAME_H_
