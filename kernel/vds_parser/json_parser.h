#ifndef __VDS_PARSER_JSON_PARSER_H_
#define __VDS_PARSER_JSON_PARSER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <stack>
#include "json_object.h"
#include "parse_error.h"
#include "encoding.h"
#include "stream.h"

namespace vds {
  class json_parser : public stream<uint8_t>
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

    json_parser(
      const std::string & stream_name,
      std::function<void(const std::shared_ptr<json_value> &)> && result,
      const options & parse_options = options())
      : stream_name_(stream_name), parse_options_(parse_options),
        result_(std::move(result)),
        state_(ST_BOF),
        current_object_(nullptr),
        line_(1), column_(1)
      {
      }

      void write(
        const service_provider & sp,
        const uint8_t * input_buffer,
        size_t input_len) override
      {
        if(nullptr == input_buffer || 0 == input_len){
          this->final_data();
          return;
        }
        
        while(0 < input_len--) {
          auto current_symbol = (char)*input_buffer++;
          switch (current_symbol) {
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
            switch (current_symbol) {
            case '/':
              this->after_slesh();
              break;

            case '[':
              this->start_array();
              this->root_object_ = this->current_object_;
              break;

            case '{':
              this->start_object();
              this->root_object_ = this->current_object_;
              break;

            default:
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;
          case ST_AFTER_SLESH:
            switch (current_symbol)
            {
            case '/':
              this->state_ = ST_INLINE_COMMENT;
              break;
            default:
              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;
          case ST_INLINE_COMMENT:
            switch (current_symbol)
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
            switch (current_symbol) {
            case ']':
              this->final_array();
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
              if (isspace(current_symbol)) {
                continue;
              }

              if (isdigit(current_symbol)) {
                this->buffer_ = current_symbol;
                this->saved_states_.push(ST_ARRAY_ITEM);
                this->state_ = ST_NUMBER;
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            };
            break;

          case ST_ARRAY_ITEM:
            switch (current_symbol) {
            case ']':
              this->final_array();
              break;

            case ',':
              this->state_ = ST_ARRAY;
              break;

            default:
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;

          case ST_OBJECT:
            switch (current_symbol) {
            case '\"':
              this->saved_states_.push(ST_OBJECT_ITEM);
              this->saved_states_.push(ST_OBJECT_PROPERTY_NAME);
              this->state_ = ST_STRING;
              break;

            case '}':
              this->final_object();
              break;

            default:
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;

          case ST_OBJECT_ITEM:
            switch (current_symbol) {
            case '}':
              this->final_object();
              break;

            case ',':
              this->state_ = ST_OBJECT;
              break;

            default:
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;

          case ST_STRING:
            switch (current_symbol) {
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
                std::static_pointer_cast<json_array>(this->current_object_)->add(
                  std::make_shared<json_primitive>(
                    this->line_, this->column_,
                    this->buffer_
                  
                ));
                this->buffer_.clear();
                break;

              default:
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + current_symbol);
              }
              break;
            default:
              this->buffer_ += current_symbol;
              break;
            }
            break;

          case ST_STRING_BACKSLESH:
            switch (current_symbol) {
              case '\"':
              case '\\':
                this->buffer_ += current_symbol;
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
                  std::string("Unexpected char ") + current_symbol);
            }
            break;
            
          case ST_STRING_SYMBOL_1:
            switch (current_symbol) {
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
                this->num_buffer_ = current_symbol - '0';
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
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + current_symbol);
            }
            break;
                
          case ST_STRING_SYMBOL_2:
            this->num_buffer_ <<= 4;
            switch (current_symbol) {
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
                this->num_buffer_ |= current_symbol - '0';
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
                  std::string("Unexpected char ") + current_symbol);
            }
            break;

                
          case ST_STRING_SYMBOL_3:
            this->num_buffer_ <<= 4;
            switch (current_symbol) {
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
                this->num_buffer_ |= current_symbol - '0';
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
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + current_symbol);
            }
            break;
                
          case ST_STRING_SYMBOL_4:
            this->num_buffer_ <<= 4;
            switch (current_symbol) {
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
                this->num_buffer_ |= current_symbol - '0';
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
                throw parse_error(
                  this->stream_name_,
                  this->line_,
                  this->column_,
                  std::string("Unexpected char ") + current_symbol);
            }
            utf8::add(this->buffer_, (wchar_t)this->num_buffer_);
            break;

          case ST_OBJECT_PROPERTY_NAME:
            switch (current_symbol) {
            case ':':
              this->state_ = ST_OBJECT_PROPERTY_VALUE;
              break;

            default:
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;

          case ST_OBJECT_PROPERTY_VALUE:
            switch (current_symbol) {
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
              if (isspace(current_symbol)) {
                continue;
              }

              throw parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + current_symbol);
            }
            break;

          default:
            throw parse_error(
              this->stream_name_,
              this->line_,
              this->column_,
              std::string("Unexpected char ") + current_symbol);
          }
        }
      }
      
    private:
      std::string stream_name_;
      std::function<void(const std::shared_ptr<json_value> &)> result_;
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

      std::shared_ptr<json_value> root_object_;
      std::stack<std::shared_ptr<json_value>> current_path_;
      std::shared_ptr<json_value> current_object_;

      int line_;
      int column_;

      std::string buffer_;
      uint32_t num_buffer_;

      void final_data()
      {
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
      }
      
      void after_slesh()
      {
        if (!this->parse_options_.enable_inline_comments) {
          throw parse_error(
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
        auto new_object = std::make_shared<json_array>(this->line_, this->column_);

        switch (this->state_) {
        case ST_OBJECT_PROPERTY_VALUE:
          std::static_pointer_cast<json_property>(this->current_object_)->value(new_object);
          break;

        case ST_BOF:
          this->saved_states_.push(ST_BOF);
          break;

        default:
          throw parse_error(
            this->stream_name_,
            this->line_,
            this->column_,
            "Unexpected state");
        }

        this->current_object_ = new_object;
        this->state_ = ST_ARRAY;
      }

      void final_array()
      {
        this->state_ = this->saved_states_.top();
        this->saved_states_.pop();

        if (ST_BOF == this->state_) {
          if (!this->parse_options_.enable_multi_root_objects) {
            this->state_ = ST_EOF;
          }
          
          this->result_(this->root_object_);
        }
        else {
          this->current_object_ = this->current_path_.top();
          this->current_path_.pop();
        }
      }

      void start_object()
      {
        auto new_object = std::make_shared<json_object>(this->line_, this->column_);

        switch (this->state_) {
        case ST_OBJECT_PROPERTY_VALUE:
          std::static_pointer_cast<json_property>(this->current_object_)->value(new_object);
          break;
        case ST_BOF:
          this->saved_states_.push(ST_BOF);
          break;
        case ST_ARRAY:
          std::static_pointer_cast<json_array>(this->current_object_)->add(new_object);
          this->saved_states_.push(ST_ARRAY_ITEM);
          this->current_path_.push(this->current_object_);
          break;
        default:
          throw parse_error(
            this->stream_name_,
            this->line_,
            this->column_,
            "Unexpected state");
        }

        this->current_object_ = new_object;
        this->state_ = ST_OBJECT;
      }

      void final_object()
      {
        this->state_ = this->saved_states_.top();
        this->saved_states_.pop();

        if (ST_BOF == this->state_) {
          if (!this->parse_options_.enable_multi_root_objects) {
            this->state_ = ST_EOF;
          }
          
          this->result_(this->root_object_);
        }
        else {
          this->current_object_ = this->current_path_.top();
          this->current_path_.pop();
        }
      }

      void start_property()
      {
        auto new_property = std::make_shared<json_property>(this->line_, this->column_);
        new_property->name(this->buffer_);

        std::static_pointer_cast<json_object>(this->current_object_)->add_property(new_property);
        this->current_path_.push(this->current_object_);

        this->current_object_ = new_property;
        this->buffer_.clear();
      }

      void final_string_property()
      {
        std::static_pointer_cast<json_property>(this->current_object_)->value(
          std::make_shared<json_primitive>(this->line_, this->column_, this->buffer_)
        );

        this->current_object_ = this->current_path_.top();
        this->current_path_.pop();

        this->buffer_.clear();
      }
  };
}

#endif // __VDS_PARSER_JSON_PARSER_H_
