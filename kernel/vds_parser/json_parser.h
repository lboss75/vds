#ifndef __VDS_PARSER_JSON_PARSER_H_
#define __VDS_PARSER_JSON_PARSER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "json_object.h"
#include "parse_error.h"

namespace vds {
  class json_parser
  {
  public:
    struct options
    {
      bool enable_inline_comments;
      bool enable_multi_root_objects;


      options()
      : enable_inline_comments(false),
        enable_multi_root_objects(false)
      {
      }
    };

    json_parser(const std::string & stream_name, const options & parse_options = options())
      : stream_name_(stream_name), parse_options_(parse_options)
    {
    }

    template <typename context_type>
    class handler : public dataflow_step<context_type, bool(json_value *)>
    {
      using base_class = dataflow_step<context_type, bool(json_value *)>;
    public:
      handler(
        const context_type & context,
        const json_parser & args
        ) : base_class(context),
        data_(nullptr), len_(0),
        stream_name_(args.stream_name_),
        parse_options_(args.parse_options_),
        state_(ST_BOF),
        current_object_(nullptr),
        line_(1), column_(1)
      {
      }

      bool operator()(
        const service_provider & sp,
        const void * data,
        size_t len
        )
      {
        if (0 == len) {
          switch (this->state_) {
          case ST_EOF:
            break;

          case ST_BOF:
            if (this->parse_options_.enable_multi_root_objects) {
              break;
            }
            //break;

          default:
            throw parse_error(
              this->stream_name_,
              this->line_,
              this->column_,
              "Unexpected end of data");
          }

          return this->next(sp, nullptr);
        }
        else {
          this->data_ = (const char *)data;
          this->len_ = len;

          return this->continue_process(sp);
        }
      }

      void processed(const service_provider & sp)
      {
        if(this->continue_process(sp)){
          this->prev(sp);
        }
      }
      
    private:
      bool continue_process(const service_provider & sp)
      {
        for (; 0 < this->len_; this->len_--, this->data_++) {
          switch (*this->data_) {
          case '\n':
            this->line_++;
            this->column_ = 1;
            break;

          case '\t':
            this->column_ = 1 + ((this->column_ + 7) & ~7);
            break;

          default:
            this->column_++;
            break;
          }

          switch (this->state_) {
          case ST_BOF:
            switch (*this->data_) {
            case '/':
              this->after_slesh();
              break;

            case '[':
              this->start_array();
              this->root_object_.reset(this->current_object_);
              break;

            case '{':
              this->start_object();
              this->root_object_.reset(this->current_object_);
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;
          case ST_AFTER_SLESH:
            switch (*this->data_)
            {
            case '/':
              this->state_ = ST_INLINE_COMMENT;
              break;
            default:
              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;
          case ST_INLINE_COMMENT:
            switch (*this->data_)
            {
            case '\n':
              this->state_ = this->saved_states_.top();
              saved_states_.pop();
              break;
            default:
              break;
            };
            break;
          case ST_ARRAY:
            switch (*this->data_) {
            case ']':
              if (this->final_array(sp)) {
                return true;
              }
              break;

            case '[':
              this->start_array();
              break;

            case '{':
              this->start_object();
              break;

            case '/':
              this->after_slesh();
              break;

            case '\"':
              this->saved_states_.push(ST_ARRAY_ITEM);
              this->state_ = ST_STRING;
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              if (isdigit(*this->data_)) {
                this->buffer_ = *this->data_;
                this->saved_states_.push(ST_ARRAY_ITEM);
                this->state_ = ST_NUMBER;
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            };
            break;

          case ST_ARRAY_ITEM:
            switch (*this->data_) {
            case ']':
              if (this->final_array(sp)) {
                return true;
              }
              break;

            case ',':
              this->state_ = ST_ARRAY;
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;

          case ST_OBJECT:
            switch (*this->data_) {
            case '\"':
              this->saved_states_.push(ST_OBJECT_ITEM);
              this->saved_states_.push(ST_OBJECT_PROPERTY_NAME);
              this->state_ = ST_STRING;
              break;

            case '}':
              if (this->final_object(sp)) {
                return true;
              }
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;

          case ST_OBJECT_ITEM:
            switch (*this->data_) {
            case '}':
              if (this->final_object(sp)) {
                return true;
              }
              break;

            case ',':
              this->state_ = ST_OBJECT;
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw new parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;

          case ST_STRING:
            switch (*this->data_) {
            case '\\':
              this->state_ = ST_STRING_BACKSLESH;
              break;
            case '\"':
              this->state_ = this->saved_states_.top();
              saved_states_.pop();
              switch (this->state_) {
              case ST_OBJECT_PROPERTY_NAME:
                this->start_property();
                break;

              case ST_OBJECT_ITEM:
                this->final_string_property();
                break;

              case ST_ARRAY_ITEM:
                static_cast<json_array *>(this->current_object_)->add(
                  new json_primitive(
                    this->line_, this->column_,
                    this->buffer_
                  )
                );
                this->buffer_.clear();
                break;

              default:
                throw new parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
              }
              break;
            default:
              this->buffer_ += *this->data_;
              break;
            }
            break;

          case ST_STRING_BACKSLESH:
            switch (*this->data_) {
              case '\"':
              case '\\':
                this->buffer_ += *this->data_;
                this->state_ = ST_STRING;
                break;
                
              case 'n':
                this->buffer_ += '\n';
                this->state_ = ST_STRING;
                break;
                
              case 'r':
                this->buffer_ += '\r';
                this->state_ = ST_STRING;
                break;
                
              case 'b':
                this->buffer_ += '\b';
                this->state_ = ST_STRING;
                break;
                
              case 't':
                this->buffer_ += '\t';
                this->state_ = ST_STRING;
                break;
                
              case 'f':
                this->buffer_ += '\f';
                this->state_ = ST_STRING;
                break;
                
              case 'x':
                this->state_ = ST_STRING_SYMBOL_1;
                break;
                
              default:
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
            }
            break;
            
          case ST_STRING_SYMBOL_1:
            switch (*this->data_) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                this->num_buffer_ = *this->data_ - '0';
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'a':
              case 'A':
                this->num_buffer_ = 10;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'b':
              case 'B':
                this->num_buffer_ = 11;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'c':
              case 'C':
                this->num_buffer_ = 12;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'd':
              case 'D':
                this->num_buffer_ = 13;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'e':
              case 'E':
                this->num_buffer_ = 14;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              case 'f':
              case 'F':
                this->num_buffer_ = 15;
                this->state_ = ST_STRING_SYMBOL_2;
                break;
                
              default:
                throw new parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
            }
            break;
                
          case ST_STRING_SYMBOL_2:
            this->num_buffer_ <<= 4;
            switch (*this->data_) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                this->num_buffer_ |= *this->data_ - '0';
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'a':
              case 'A':
                this->num_buffer_ |= 10;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'b':
              case 'B':
                this->num_buffer_ |= 11;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'c':
              case 'C':
                this->num_buffer_ |= 12;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'd':
              case 'D':
                this->num_buffer_ |= 13;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'e':
              case 'E':
                this->num_buffer_ |= 14;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              case 'f':
              case 'F':
                this->num_buffer_ |= 15;
                this->state_ = ST_STRING_SYMBOL_3;
                break;
                
              default:
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
            }
            break;

                
          case ST_STRING_SYMBOL_3:
            this->num_buffer_ <<= 4;
            switch (*this->data_) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                this->num_buffer_ |= *this->data_ - '0';
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'a':
              case 'A':
                this->num_buffer_ |= 10;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'b':
              case 'B':
                this->num_buffer_ |= 11;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'c':
              case 'C':
                this->num_buffer_ |= 12;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'd':
              case 'D':
                this->num_buffer_ |= 13;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'e':
              case 'E':
                this->num_buffer_ |= 14;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              case 'f':
              case 'F':
                this->num_buffer_ |= 15;
                this->state_ = ST_STRING_SYMBOL_4;
                break;
                
              default:
                throw new parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
            }
            break;
                
          case ST_STRING_SYMBOL_4:
            this->num_buffer_ <<= 4;
            switch (*this->data_) {
              case '0':
              case '1':
              case '2':
              case '3':
              case '4':
              case '5':
              case '6':
              case '7':
              case '8':
              case '9':
                this->num_buffer_ |= *this->data_ - '0';
                this->state_ = ST_STRING;
                break;
                
              case 'a':
              case 'A':
                this->num_buffer_ |= 10;
                this->state_ = ST_STRING;
                break;
                
              case 'b':
              case 'B':
                this->num_buffer_ |= 11;
                this->state_ = ST_STRING;
                break;
                
              case 'c':
              case 'C':
                this->num_buffer_ |= 12;
                this->state_ = ST_STRING;
                break;
                
              case 'd':
              case 'D':
                this->num_buffer_ |= 13;
                this->state_ = ST_STRING;
                break;
                
              case 'e':
              case 'E':
                this->num_buffer_ |= 14;
                this->state_ = ST_STRING;
                break;
                
              case 'f':
              case 'F':
                this->num_buffer_ |= 15;
                this->state_ = ST_STRING;
                break;
                
              default:
                throw new parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + *this->data_);
            }
            utf8::add(this->buffer_, (wchar_t)this->num_buffer_);
            break;

          case ST_OBJECT_PROPERTY_NAME:
            switch (*this->data_) {
            case ':':
              this->state_ = ST_OBJECT_PROPERTY_VALUE;
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;

          case ST_OBJECT_PROPERTY_VALUE:
            switch (*this->data_) {
            case '\"':
              this->state_ = ST_STRING;
              break;

            case '{':
              this->start_object();
              break;

            case '[':
              this->start_array();
              break;

            default:
              if (isspace(*this->data_)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *this->data_);
            }
            break;

          default:
            throw parse_error(
              this->stream_name_,
              this->line_,
              this->column_,
              std::string("Unexpected char ") + *this->data_);
          }
        }

        return true;
      }

    private:
      const char * data_;
      size_t len_;

      std::string stream_name_;
      options parse_options_;

      enum State
      {
        ST_BOF,

        ST_AFTER_SLESH,
        ST_INLINE_COMMENT,

        ST_ARRAY,
        ST_ARRAY_ITEM,

        ST_OBJECT,
        ST_OBJECT_ITEM,

        ST_NUMBER,

        ST_OBJECT_PROPERTY_NAME,
        ST_OBJECT_PROPERTY_VALUE,
        //ST_OBJECT_PROPERTY_VALUE_FINISH,

        ST_STRING,
        ST_STRING_BACKSLESH,
        
        ST_STRING_SYMBOL_1,
        ST_STRING_SYMBOL_2,
        ST_STRING_SYMBOL_3,
        ST_STRING_SYMBOL_4,

        ST_EOF
      };

      State state_;
      std::stack<State> saved_states_;

      std::unique_ptr<json_value> root_object_;
      std::stack<json_value *> current_path_;
      json_value * current_object_;

      int line_;
      int column_;

      std::string buffer_;
      uint32_t num_buffer_;

      void after_slesh()
      {
        if (!this->parse_options_.enable_inline_comments) {
          throw new parse_error(
            this->stream_name_,
            this->line_,
            this->column_,
            "Unexpected char /");
        }

        this->saved_states_.push(this->state_);
        this->state_ = ST_AFTER_SLESH;
      }

      void start_array()
      {
        auto new_object = new json_array(this->line_, this->column_);

        switch (this->state_) {
        case ST_OBJECT_PROPERTY_VALUE:
          static_cast<json_property *>(this->current_object_)->value(new_object);
          break;

        case ST_BOF:
          this->saved_states_.push(ST_BOF);
          break;

        default:
          throw new parse_error(
            this->stream_name_,
            this->line_,
            this->column_,
            "Unexpected state");
        }

        this->current_object_ = new_object;
        this->state_ = ST_ARRAY;
      }

      bool final_array(const service_provider & sp)
      {
        this->state_ = this->saved_states_.top();
        this->saved_states_.pop();

        if (ST_BOF == this->state_) {
          if (!this->parse_options_.enable_multi_root_objects) {
            this->state_ = ST_EOF;
          }
          this->len_--;
          this->data_++;
          this->next(sp, this->root_object_.release());
          return true;
        }
        else {
          this->current_object_ = this->current_path_.top();
          this->current_path_.pop();
        }
        return false;
      }

      void start_object()
      {
        auto new_object = new json_object(this->line_, this->column_);

        switch (this->state_) {
        case ST_OBJECT_PROPERTY_VALUE:
          static_cast<json_property *>(this->current_object_)->value(new_object);
          break;
        case ST_BOF:
          this->saved_states_.push(ST_BOF);
          break;
        case ST_ARRAY:
          static_cast<json_array *>(this->current_object_)->add(new_object);
          this->saved_states_.push(ST_ARRAY_ITEM);
          this->current_path_.push(this->current_object_);
          break;
        default:
          throw new parse_error(
            this->stream_name_,
            this->line_,
            this->column_,
            "Unexpected state");
        }

        this->current_object_ = new_object;
        this->state_ = ST_OBJECT;
      }

      bool final_object(const service_provider & sp)
      {
        this->state_ = this->saved_states_.top();
        this->saved_states_.pop();

        if (ST_BOF == this->state_) {
          if (!this->parse_options_.enable_multi_root_objects) {
            this->state_ = ST_EOF;
          }
          this->len_--;
          this->data_++;
          this->next(sp, this->root_object_.release());
          return true;
        }
        else {
          this->current_object_ = this->current_path_.top();
          this->current_path_.pop();
        }
        return false;
      }

      void start_property()
      {
        auto new_property = new json_property(this->line_, this->column_);
        new_property->name(this->buffer_);

        static_cast<json_object *>(this->current_object_)->add_property(new_property);
        this->current_path_.push(this->current_object_);

        this->current_object_ = new_property;
        this->buffer_.clear();
      }

      void final_string_property()
      {
        static_cast<json_property *>(this->current_object_)->value(
          new json_primitive(this->line_, this->column_, this->buffer_)
        );

        this->current_object_ = this->current_path_.top();
        this->current_path_.pop();

        this->buffer_.clear();
      }
    };
    
  private:
    std::string stream_name_;
    options parse_options_;
  };

  class json_require_once
  {
  public:

    template <typename context_type>
    class handler : public dataflow_step<context_type, bool(json_value *)>
    {
      using base_class = dataflow_step<context_type, bool(json_value *)>;
    public:
      handler(
        const context_type & context,
        const json_require_once & args
      ) : base_class(context)
      {

      }

      bool operator()(const service_provider & sp, json_value * item)
      {
        if (nullptr == item) {
          if (!this->result_) {
            this->error(sp, std::make_exception_ptr(std::runtime_error("JSON object is required")));
            return false;
          }
          else {
            return this->next(sp, this->result_.release());
          }
        }
        else {
          if (!this->result_) {
            this->result_.reset(item);
            return true;
          }
          else {
            delete item;
            this->error(sp, std::make_exception_ptr(std::runtime_error("Only one JSON object has expected")));
            return false;
          }
        }
      }

    private:
      std::unique_ptr<json_value> result_;
    };

  };
}

#endif // __VDS_PARSER_JSON_PARSER_H_
