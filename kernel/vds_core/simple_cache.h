#ifndef __VDS_CORE_SIMPLE_CACHE_H_
#define __VDS_CORE_SIMPLE_CACHE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <typename key_type, typename value_type, size_t max_count = 1000, size_t remove_count = max_count / 10>
  class simple_cache
  {
  public:
    typedef std::list<std::pair<key_type, value_type>> data_type;

    typename data_type::iterator find(const key_type & key)
    {
      for (typename data_type::iterator p = this->data_.begin(); p != this->data_.end(); ++p) {
        if (p->first == key) {
          this->data_.splice(this->data_.begin(), this->data_, p);
          return this->data_.begin();
        }
      }

      return this->data_.end();
    }

    typename data_type::iterator end()
    {
      return this->data_.end();
    }

    value_type & set(const key_type & key, const value_type & value)
    {
      auto p = this->find(key);
      if (this->data_.end() == p) {
        this->data_.push_front(std::pair<key_type, value_type>(key, value));

        if (this->data_.size() > max_count) {
          while (this->data_.size() > (max_count - remove_count)) {
            this->data_.pop_back();
          }
        }
        return this->data_.begin()->second;
      }
      else {
        p->second = value;
        return p->second;
      }
    }

    value_type & set(const key_type & key, value_type && value)
    {
      auto p = this->find(key);
      if (this->data_.end() == p) {
        this->data_.push_front(std::pair<key_type, value_type>(key, std::move(value)));

        if (this->data_.size() > max_count) {
          while (this->data_.size() > (max_count - remove_count)) {
            this->data_.pop_back();
          }
        }

        return this->data_.begin()->second;
      }
      else {
        p->second = std::move(value);
        return p->second;
      }
    }
    
    void remove(const key_type & key)
    {
      this->data_.remove_if(
        [key](const std::pair<key_type, value_type> & item){
          return item.first == key;
      });
    }

  private:
    data_type data_;
  };

}


#endif//__VDS_CORE_SIMPLE_CACHE_H_
