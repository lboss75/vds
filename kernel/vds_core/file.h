#ifndef __VDS_CORE_FILE_H_
#define __VDS_CORE_FILE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "targetver.h"
#include <fcntl.h>
#include <exception>
#include <system_error>

#include "filename.h"
#include "dataflow.h"
#include "const_data_buffer.h"

namespace vds {

  class file
  {
  public:
    enum file_mode
    {
      //Opens the file if it exists and seeks to the end of the file, or creates a new file.
      append,

      //Specifies that the operating system should open an existing file
      open_read,

      //Specifies that the operating system should open an existing file
      open_write,

      //Specifies that the operating system should open an existing file
      open_read_and_write,

      //Specifies that the operating system should open a file if it exists; otherwise, a new file should be created
      open_or_create,

      //Specifies that the operating system should create a new file
      create,

      //Specifies that the operating system should create a new file
      create_new,

      //Specifies that the operating system should open an existing file
      truncate
    };


    file();
    file(const filename & filename, file_mode mode);
    ~file();

    void open(const filename & filename, file_mode mode);
    void close();
    
    size_t read(void * buffer, size_t buffer_len);
    void write(const void * buffer, size_t buffer_len);
    void write(const const_data_buffer & buf) { this->write(buf.data(), buf.size()); }

    size_t length() const;

    static size_t length(const filename & fn);
    static bool exists(const filename & fn);

    void flush();

    static void move(const filename & source, const filename & target);
    static void delete_file(const filename & fn, bool ignore_error = false);
    static std::string read_all_text(const filename & fn);
    static const_data_buffer read_all(const filename & fn);

  private:
    filename filename_;
    int handle_;
  };

  class output_text_stream
  {
  public:
    output_text_stream(file & f);
    ~output_text_stream();

    void write(const std::string & value);
    void flush();

  private:
    file & f_;
    char buffer_[4096];
    size_t written_;
  };

  class input_text_stream
  {
  public:
    input_text_stream(file & f);

    bool read_line(std::string & result);

  private:
    file & f_;
    char buffer_[4096];
    size_t offset_;
    size_t readed_;
  };

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
        throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
      }
#else
      this->handle_ = _open(this->filename_.local_name().c_str(), _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
      if (0 > this->handle_) {
        auto error = GetLastError();
        throw std::system_error(error, std::system_category(), "Unable to open file " + this->filename_.str());
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
    bool read(const service_provider & sp, next_step_t & next)
    {
      auto readed = ::read(this->handle_, this->buffer_, sizeof(this->buffer_));
      if (0 > readed) {
#ifdef _WIN32
        auto error = GetLastError();
#else
        auto error = errno;
#endif
        throw std::system_error(error, std::system_category(), "Unable to read file " + this->filename_.str());
      }

      if (0 < readed) {
        next(sp, this->buffer_, readed);
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

    using outgoing_item_type = uint8_t;
    template <typename context_type>
    class handler : public sync_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const read_file & args
        ) : base_class(context),
            f_(args.filename_, file::open_read)
      {
      }

      size_t sync_get_data(const service_provider & sp, uint8_t * buffer, size_t buffer_size)
      {
        return this->f_.read(buffer, buffer_size);
      }

    private:
      file f_;
    };

  private:
    filename filename_;
  };
  
  class file_range_read
  {
  public:
    file_range_read(const filename & filename, size_t start_offset, size_t max_size)
      : filename_(filename)
    {
    }

    using outgoing_item_type = uint8_t;
    template <typename context_type>
    class handler : public sync_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const read_file & args
        ) : base_class(context),
            f_(args.filename_, file::open_read)
      {
      }

      size_t sync_get_data(const service_provider & sp, uint8_t * buffer, size_t buffer_size)
      {
        return this->f_.read(buffer, buffer_size);
      }

    private:
      file f_;
    };

  private:
    filename filename_;
    size_t start_offset_;
    size_t max_size_;
  };
  
  class write_file
  {
  public:
    write_file(
      const filename & filename,
      file::file_mode mode)
    : filename_(filename),
    mode_(mode)
    {
    }
    
    using incoming_item_type = uint8_t;
    template <typename context_type>
    class handler : public sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const write_file & args
        ) : base_class(context),
        f_(args.filename_, args.mode_)
      {
      }

      size_t sync_push_data(const service_provider & sp, const incoming_item_type * data, size_t size)
      {
        this->f_.write(data, size);
        return size;
      }
      
      void final_data(const service_provider & sp)
      {
        this->f_.close();
      }
      
    private:
      file f_;
    };
    
  private:
    filename filename_;
    file::file_mode mode_;
  };

  class append_to_file
  {
  public:
    append_to_file(file & target)
      : target_(target)
    {
    }

    using incoming_item_type = uint8_t;
    
    template <typename context_type>
    class handler : public sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const append_to_file & args
      ) : base_class(context),
        target_(args.target_)
      {
      }

      size_t sync_push_data(const service_provider & sp, const incoming_item_type * data, size_t size)
      {
        this->f_.write(data, size);
        return size;
      }

      void final_data(const service_provider & sp)
      {
        this->f_.close();
      }

    private:
      file & target_;
    };

  private:
    file & target_;
  };
}

#endif//__VDS_CORE_FILE_H_
