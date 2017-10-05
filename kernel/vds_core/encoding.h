#ifndef __VDS_CORE_ENCODING_H_
#define __VDS_CORE_ENCODING_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "types.h"
#include "const_data_buffer.h"

namespace vds {
  class utf8
  {
  public:
    static size_t char_size(char first_utf8_symbol);
    static wchar_t next_char(const char *& utf8string, size_t & len);
    static void add(std::string & result, wchar_t ch);

  };

  class utf16
  {
  public:
    static std::wstring from_utf8(const std::string & original);
    static std::string to_utf8(const std::wstring & original);
  };
  
  class base64
  {
  public:
    static std::string from_bytes(const void * data, size_t len);
    static std::string from_bytes(const const_data_buffer & data);
    static const_data_buffer to_bytes(const std::string & data);
  };
  
  /*
  class utf8_byte_to_wchar
  {
  public:
    using incoming_item_type = uint8_t;
    using outgoing_item_type = wchar_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 4;

    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
    public:
      handler(
        const context_type & context,
        const utf8_byte_to_wchar & args)
        : vds::sync_dataflow_filter<context_type, handler<context_type>>(context),
        written_(0)
      {
      }

      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        if (0 < this->written_) {
          auto required = utf8::char_size(this->buffer_[0]);
          auto n = this->input_buffer_size();
          if (n > required - this->written_) {
            n = required - this->written_;
          }

          memcpy(this->buffer_ + this->written_, this->input_buffer(), n);
          const char * buf = (const char *)this->buffer_;
          this->output_buffer(0) = utf8::next_char(buf, this->written_);
          input_readed = n;
          output_written = 1;
        }
        else
        {
          input_readed = 0;
          output_written = 0;
        }

        while(input_readed < this->input_buffer_size() && output_written < this->output_buffer_size()){
          auto required = utf8::char_size(this->input_buffer(input_readed));
          if(required > this->input_buffer_size()) {
            memcpy(this->buffer_, this->input_buffer() + input_readed, this->input_buffer_size());
            this->written_ = this->input_buffer_size();
            input_readed += this->input_buffer_size();
            break;
          }

          const char * buf = (const char *)this->input_buffer() + input_readed;
          input_readed += required;
          this->output_buffer(output_written) = utf8::next_char(buf, required);
          output_written++;
        }
      }

    private:
      uint8_t buffer_[4];
      size_t written_;
    };

  };
  
  
  class byte_to_char
  {
  public:
    using incoming_item_type = uint8_t;
    using outgoing_item_type = char;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 1;

    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
    public:
      handler(
        const context_type & context,
        const byte_to_char & args)
        : vds::sync_dataflow_filter<context_type, handler<context_type>>(context)
      {
      }

      void sync_process_data(const vds::service_provider & sp, size_t & input_readed, size_t & output_written)
      {
        auto n = (this->output_buffer_size() < this->input_buffer_size()) ? this->output_buffer_size() : this->input_buffer_size();
        memcpy(this->output_buffer(), this->input_buffer(), n);
        input_readed = output_written = n;
      }
    };

  };
  */
}

#endif//__VDS_CORE_ENCODING_H_
