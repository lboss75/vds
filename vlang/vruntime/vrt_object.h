#ifndef __VRT_OBJECT_H_
#define __VRT_OBJECT_H_

namespace vds {
  class vrt_type;

  class vrt_object : public std::enable_shared_from_this<vrt_object>
  {
  public:
    vrt_object(const vrt_type * type);
    
    const vrt_type * get_type() const {
      return this->type_;
    }

    template<typename T>
    void add_tag(const std::string & name, const std::shared_ptr<T> & value)
    {
      auto v = static_cast<value_hodler<T> *>(this->tags_[types::get_type_id<T>()][name].get());
      if(nullptr == v){
        this->tags_[types::get_type_id<T>()][name].reset(new value_hodler<T>(value));
      }
      else {
        v->value(value);
      }
    }

    template<typename T>
    std::shared_ptr<T> get_tag(const std::string & name) const
    {
      auto v = static_cast<value_hodler<T> *>(this->tags_[types::get_type_id<T>()][name].get());
      if(nullptr == v){
        return std::shared_ptr<T>();
      }
      
      return v->value();
    }

  private:
    const vrt_type * type_;

    class holder
    {
    public:
      virtual ~holder() {}

    };
    
    template <typename T>
    class value_hodler : public holder
    {
    public:
      value_hodler()
      {
      }
      
      value_hodler(const std::shared_ptr<T> & value)
      : value_(value)
      {
      }
      
      const std::shared_ptr<T> & value() const {
        return this->value_;
      }
      
      void value(const std::shared_ptr<T> & value) {
        this->value_ = value;
      }
      
    private:
      std::shared_ptr<T> value_;
    };

    std::map<int, std::map<std::string, std::unique_ptr<holder>>> tags_;
  };
  
  class vrt_string_object : public vrt_object
  {
  public:
    vrt_string_object(const vrt_type * type, const std::string & value);
    
    const std::string & value() const {
      return this->value_;
    }
  private:
    std::string value_;
  };
}

#endif // __VRT_OBJECT_H_
