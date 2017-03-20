#ifndef __VDS_DATA_FILE_CONTAINER_H_
#define __VDS_DATA_FILE_CONTAINER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class file_container
  {
  public:
    file_container& add(const std::string & name, const std::string & body)
    {
      this->items_.push_back(item {item::it_string, name, body });
      return *this;
    }
    
  private:
    struct item
    {
    public:
      enum item_type
      {
        it_string,
        it_text_file,
        it_binary_file
      };
      
      item_type type;
      std::string name;
      std::string string_body;
      filename file_name;
    };
    
    std::list<item> items_;

  public:

    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const file_container & args)
        : base_class(context),
        items_(args.items_),
        current_(items_.begin())
      {
      }

      void operator()()
      {
        this->processed();
      }

      void processed()
      {
        if (this->items_.end() == this->current_) {
          this->next(nullptr, 0);
        }
        else {
          if (item::item_type::it_string == this->current_->type) {
            binary_serializer s;

            do {
              s << (uint8_t)1 << this->current_->name << this->current_->string_body;
              this->current_++;
            } while (this->items_.end() != this->current_
              && item::item_type::it_string == this->current_->type);

            s << (uint8_t)0;

            this->next(s.data().data(), s.data().size());
          }
          else {
            throw new std::runtime_error("Not implemented");
          }
        }
      }

    private:
      std::list<item> items_;
      std::list<item>::const_iterator current_;
    };

  };
}

#endif // __VDS_DATA_FILE_CONTAINER_H_
