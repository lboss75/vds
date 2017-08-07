#ifndef __VDS_CORE_FOLDERNAME_H_
#define __VDS_CORE_FOLDERNAME_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <functional>
#include <algorithm>

namespace vds {
  class filename;
  
  class foldername
  {
  public:
      foldername()
      {
      }
      
      foldername(const foldername& other)
      : value_(other.value_)
      {
      }
      
      explicit foldername(const std::string & str)
      : value_(str)
      {
#ifdef _WIN32
        std::replace(this->value_.begin(), this->value_.end(), '\\', '/');
#endif
      }
      
      foldername(
        const foldername& base,
        const std::string & relative);
      
      foldername& operator=(const foldername& other)
      {      
        this->value_ = other.value_;
        return *this;
      }
      
      bool operator == (const foldername& other) const
      {
        return this->value_ == other.value_;
      }

      bool operator != (const foldername& other) const
      {
        return this->value_ != other.value_;
      }

      const std::string & str() const {
        return this->value_;
      }
      
      void folders(
        const std::function<bool(const foldername & name)> & callback
      ) const;

      void files(
        const std::function<bool(const filename & name)> & callback
      ) const;

      void create();
      void delete_folder(bool recurcive) const;

      std::string relative_path(const filename & fn, bool allow_pass_border = false) const;

      const std::string & full_name() const { return this->value_; }

      std::string name() const;

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

      bool empty() const { return this->value_.empty(); }

      bool exist() const;

      foldername contains_folder() const;

  private:
    friend class filename;
    std::string value_;
  };
}

#endif // __VDS_CORE_FOLDERNAME_H_
