#ifndef __VDS_CORE_FILENAME_H_
#define __VDS_CORE_FILENAME_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <string>
#include <algorithm>
#include "expected.h"

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
      
      
      explicit filename(const std::string & filepath)
      : value_(filepath)
      {
#ifdef _WIN32
        std::replace(this->value_.begin(), this->value_.end(), '\\', '/');
#endif
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
      
      foldername contains_folder() const;

      const std::string & full_name() const { return this->value_; }
      std::string name() const;
      std::string name_without_extension() const;
      std::string extension() const;

      const std::string & str() const
      {
        return this->value_;
      }

      std::string local_name() const
      {
#ifndef _WIN32
        return this->value_;
#else
        auto result = this->value_;
        std::replace(result.begin(), result.end(), '/', '\\');
        return result;
#endif
      }

      bool empty() const
      {
        return this->value_.empty();
      }

      static expected<filename> current_process();
  private:
    friend class foldername;
    std::string value_;
  };
}

#endif // __VDS_CORE_FILENAME_H_
