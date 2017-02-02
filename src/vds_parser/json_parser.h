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
    class handler : public sequence_step<context_type, void(std::unique_ptr<json_object> &)>
    {
      using base_class = sequence_step<context_type, void(std::unique_ptr<json_object> &)>;
    public:
      handler(
        const context_type & context,
        const json_parser & args
        ) : base_class(context),
        stream_name_(args.stream_name_),
        parse_options_(args.parse_options_),
        state_(ST_BOF), line_(1), column_(1)
      {

      }

      void operator()(
        const char * data,
        size_t len
        )
      {
        for (size_t i = 0ul; i < len; ++i, ++data) {
          switch (*data) {
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
            switch (*data) {
            case '/':
              this->after_slesh();
              break;

            case '[':
              this->start_array();
              break;

            case '{':
              this->start_object();
              break;

            default:
              if (isspace(*data)) {
                continue;
              }

              throw new parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *data);
            }
            break;
          case ST_AFTER_SLESH:
            switch (*data)
            {
            case '/':
              this->state_ = ST_INLINE_COMMENT;
              break;
            default:
              throw new parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *data);
            }
            break;
          case ST_INLINE_COMMENT:
            switch (*data)
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
            switch (*data) {
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
              this->start_string();
              break;

            default:
              if (isspace(*data)) {
                continue;
              }

              if (isdigit(*data)) {
                this->start_number();
                continue;
              }

              throw new parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *data);
            };
            break;

          case ST_ARRAY_ITEM:
            switch (*data)
            {
            case ']':
              this->final_array();
              break;

            case ',':
              this->state_ = ST_ARRAY;
              break;

            default:
              if (isspace(*data)) {
                continue;
              }

              throw new parse_error(
                this->stream_name_,
                this->line_,
                this->column_,
                std::string("Unexpected char ") + *data);
            }
            break;

          default:
            break;
          }
        }
      }

    private:
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

        ST_EOF
      };

      State state_;
      std::stack<State> saved_states_;

      std::unique_ptr<json_value> root_object_;
      std::stack<json_value *> current_path_;
      json_value * current_object_;

      int line_;
      int column_;

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
        static_cast<json_array *>(this->current_object_)->add(new_object);
        this->current_path_.push(this->current_object_);
        this->current_object_ = new_object;

        this->saved_states_.push(ST_ARRAY_ITEM);
        this->state_ = ST_ARRAY;
      }

      void final_array()
      {
        this->state_ = this->saved_states_.top();
        this->saved_states_.top();

        if (ST_BOF == this->state_) {
          this->next(this->root_object_.release());
          if (!this->parse_options_.enable_multi_root_objects) {
            this->state_ = ST_EOF;
          }
        }
        else {
          this->current_object_ = this->current_path_.top();
          this->current_path_.pop();
        }
      }

      void start_object()
      {
        auto new_object = new json_object(this->line_, this->column_);
        static_cast<json_array *>(this->current_object_)->add(new_object);
        this->current_path_.push(this->current_object_);
        this->current_object_ = new_object;

        this->saved_states_.push(ST_ARRAY_ITEM);
        this->state_ = ST_OBJECT;
      }

      void final_object()
      {
      }

      void start_string()
      {

      }

      void start_number()
      {

      }


    };
    
  private:
    std::string stream_name_;
    options parse_options_;
  };
}

#endif // __VDS_PARSER_JSON_PARSER_H_
