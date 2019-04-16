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
#include "const_data_buffer.h"
#include "expected.h"


namespace vds {

  class file
  {
  public:
    enum class file_mode
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
      create_new,

      //Specifies that the operating system should open an existing file
      truncate
    };


    file();
    file(const file & f) = delete;
    file(file && f) noexcept;
    ~file();
    
    expected<void> open(const filename & filename, file_mode mode);
    expected<void> close();
    
    expected<size_t> read(
      void * buffer,
      size_t buffer_len);

    expected<void> write(
      const void * buffer,
      size_t buffer_len);

    expected<void> write(
      const const_data_buffer & buf) {
      return this->write(buf.data(), buf.size());
    }

    const filename & name() const {
      return this->filename_;
    }

    file & operator = (const file & f) = delete;
    file & operator = (file && f) noexcept;

    static expected<file> create_temp(const service_provider * sp);

    expected<size_t> length() const;

    expected<void> seek(size_t position);

    static expected<size_t> length(const filename & fn);
    static bool exists(const filename & fn);

    expected<void> flush();

    static expected<void> copy(const filename & source, const filename & target, bool override_exist);
    static expected<void> move(const filename & source, const filename & target);
    static expected<void> delete_file(const filename & fn);
    static expected<std::string> read_all_text(const filename & fn);
    static expected<const_data_buffer> read_all(const filename & fn);
    static expected<void> write_all(const filename & fn, const const_data_buffer & data);

  private:
    filename filename_;
    int handle_;
  };

  class output_text_stream
  {
  public:
    output_text_stream(file & f);
    ~output_text_stream();

    expected<void> write(
      const std::string & value);
    
    expected<void> flush();

  private:
    file & f_;
    char buffer_[4096];
    size_t written_;
  };

  class input_text_stream
  {
  public:
    input_text_stream(file & f);

    expected<bool> read_line(std::string & result);

  private:
    file & f_;
    char buffer_[4096];
    size_t offset_;
    size_t readed_;
  };
/*
  class file_read
  {
  public:
    file_read(const filename & filename)
      : filename_(filename)
    {
    }

    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template <typename context_type>
    class handler : public sync_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const file_read & args
        ) : base_class(context),
            f_(args.filename_, file::file_mode::open_read)
      {
      }

      size_t sync_get_data()
      {
        if (0 == this->output_buffer_size()) {
          this->f_.close();
          return 0;
        }
        else {
          return this->f_.read(this->output_buffer(), this->output_buffer_size());
        }
      }

    private
      file f_;
    };

  private
    filename filename_;
  };
  
  class file_range_read
  {
  public:
    file_range_read(const filename & filename, size_t start_offset, size_t max_size)
      : filename_(filename), start_offset_(start_offset), max_size_(max_size)
    {
    }

    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template <typename context_type>
    class handler : public sync_dataflow_source<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_source<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const file_range_read & args)
        : base_class(context),
          f_(args.filename_, file::file_mode::open_read),
          start_offset_(args.start_offset_),
          max_size_(args.max_size_)
      {
        this->f_.seek(this->start_offset_);
      }

      size_t sync_get_data()
      {
        if (0 == this->output_buffer_size() || 0 == this->max_size_) {
          this->f_.close();
          return 0;
        }
        else {
          auto n = this->output_buffer_size();
          if (n > this->max_size_) {
            n = this->max_size_;
          }

          auto readed = this->f_.read(this->output_buffer(), n);
          this->max_size_ -= readed;
          return readed;
        }
      }

    private
      file f_;
      size_t start_offset_;
      size_t max_size_;
    };

  private
    filename filename_;
    size_t start_offset_;
    size_t max_size_;
  };
  
  class file_write
  {
  public:
    file_write(
      const filename & filename,
      file::file_mode mode)
    : filename_(filename),
    mode_(mode)
    {
    }
    
    using incoming_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template <typename context_type>
    class handler : public sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const file_write & args
        ) : base_class(context),
        f_(args.filename_, args.mode_)
      {
      }

      size_t sync_push_data()
      {
        if(0 == this->input_buffer_size()){
          this->f_.close();
        }
        else {
          this->f_.write(this->input_buffer(), this->input_buffer_size());
        }

        return this->input_buffer_size();
      }
      
    private
      file f_;
    };
    
  private
    filename filename_;
    file::file_mode mode_;
  };

  class file_append
  {
  public:
    file_append(file & target)
      : target_(target)
    {
    }

    using incoming_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 4 * 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1024;

    template <typename context_type>
    class handler : public sync_dataflow_target<context_type, handler<context_type>>
    {
      using base_class = sync_dataflow_target<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const file_append & args
      ) : base_class(context),
        target_(args.target_)
      {
      }

      size_t sync_push_data()
      {
        if (0 != this->input_buffer_size()) {
          this->target_.write(this->input_buffer(), this->input_buffer_size());
        }

        return this->input_buffer_size();
      }

    private
      file & target_;
    };

  private
    file & target_;
  };
  */
}

#endif//__VDS_CORE_FILE_H_
