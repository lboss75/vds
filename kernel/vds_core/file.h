#ifndef __VDS_CORE_FILE_H_
#define __VDS_CORE_FILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "filename.h"

namespace vds {

  class read_file_stream
  {
  public:
    read_file_stream(const filename & filename)
      : filename_(filename)
    {
#ifndef _WIN32
      this->handle_ = open(filename.local_name().c_str(), O_RDONLY);
      if (0 > this->handle_) {
        auto error = errno;
        throw new std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
      }
#else
      this->handle_ = _open(this->filename_.local_name().c_str(), _O_RDONLY);
      if (0 > this->handle_) {
        auto error = GetLastError();
        throw new std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
      }
#endif
    }

    ~read_file_stream()
    {
#ifndef _WIN32
      close(this->handle_);
#else
      _close(this->handle_);
#endif
    }

    template<typename next_step_t>
    bool read(next_step_t & next)
    {
      auto readed = ::read(this->handle_, this->buffer_, sizeof(this->buffer_));
      if (0 > readed) {
#ifdef _WIN32
        auto error = GetLastError();
#else
        auto error = errno;
#endif
        throw new std::system_error(error, std::system_category(), "Unable to read file " + this->filename_.str());
      }

      if (0 < readed) {
        next(this->buffer_, readed);
      }
      
      return (0 < readed);
    }

  private:
    filename filename_;
    int handle_;
    char buffer_[4096];
  };

  class read_file
  {
  public:
    read_file(const filename & filename)
      : filename_(filename)
    {
    }

    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const read_file & args
        ) : base_class(context)
      {
      }


    private:
      int handle_;
      char buffer_[4096];
    };

  private:
    filename filename_;
  };
}

#endif//__VDS_CORE_FILE_H_
