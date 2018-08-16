#ifndef __VDS_CORE_BINARY_SERIALIZE_H_
#define __VDS_CORE_BINARY_SERIALIZE_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <list>
#include <set>
#include <map>
#include "types.h"
#include "const_data_buffer.h"
#include "resizable_data_buffer.h"

namespace vds {

  class binary_serializer
  {
  public:
    //bool
    binary_serializer & operator << (bool value);

    //1 byte
    binary_serializer & operator << (uint8_t value);

    //2 byte
    binary_serializer & operator << (uint16_t value);

    //4 bytes
    binary_serializer & operator << (uint32_t value);

    //8 bytes
    binary_serializer & operator << (uint64_t value);

    binary_serializer & write_number(uint64_t value);

    binary_serializer & operator << (const std::string & value);
    
    binary_serializer & push_data(const void * data, size_t size, bool serialize_size = true);
    
    binary_serializer & operator << (const const_data_buffer & data);

    const uint8_t * get_buffer() const {
      return this->data_.data();
    }

    const_data_buffer move_data() {
      return this->data_.move_data();
    }

    size_t size() const { return this->data_.size(); }
    
    template <typename T>
    binary_serializer & operator << (const std::list<T> & value)
    {
      this->write_number(value.size());
      for(auto & p : value){
        *this << p;
      }
      
      return *this;
    }

    template <typename T>
    binary_serializer & operator << (const std::vector<T> & value)
    {
      this->write_number(value.size());
      for(auto & p : value){
        *this << p;
      }

      return *this;
    }

    template <typename T>
    binary_serializer & operator << (const std::set<T> & value)
    {
      this->write_number(value.size());
      for(auto & p : value){
        *this << p;
      }

      return *this;
    }

    template <typename TKey, typename TValue>
    binary_serializer & operator << (const std::map<TKey, TValue> & value)
    {
      this->write_number(value.size());
      for(const auto & p : value){
        *this << p.first << p.second;
      }

      return *this;
    }

  private:
    resizable_data_buffer data_;

    binary_serializer & operator << (const uint8_t * value);//to avoid convert uint8_t * to std::string
  };
  
  class binary_deserializer
  {
  public:
    binary_deserializer(const const_data_buffer & data);
    binary_deserializer(const_data_buffer && data) = delete;
    binary_deserializer(const void * data, size_t len);
    
    //1 byte
    binary_deserializer & operator >> (bool & value);

    //1 byte
    binary_deserializer & operator >> (uint8_t & value);

    //2 byte
    binary_deserializer & operator >> (uint16_t & value);

    //4 byte
    binary_deserializer & operator >> (uint32_t & value);

    //8 byte
    binary_deserializer & operator >> (uint64_t & value);

    binary_deserializer & operator >> (std::string & value);
   
    binary_deserializer & operator >> (const_data_buffer & data);
    
    uint64_t read_number();

    const uint8_t * data() const { return this->data_; }
    size_t size() const { return this->len_; }
    
    void pop_data(void * data, size_t & size, bool serialize_size);
    size_t pop_data(void * data, size_t size);

    template <typename T>
    binary_deserializer & operator >> (std::list<T> & value)
    {
      auto count = this->read_number();
      for(decltype(count) i = 0; i < count; ++i){
        T item;
        *this >> item;
        value.push_back(item);
      }
      
      return *this;
    }

    template <typename T>
    binary_deserializer & operator >> (std::vector<T> & value)
    {
      auto count = this->read_number();
      for(decltype(count) i = 0; i < count; ++i){
        T item;
        *this >> item;
        value.push_back(item);
      }

      return *this;
    }

    template <typename T>
    binary_deserializer & operator >> (std::set<T> & value)
    {
      auto count = this->read_number();
      for(decltype(count) i = 0; i < count; ++i){
        T item;
        *this >> item;
        value.emplace(item);
      }

      return *this;
    }

    template <typename TKey, typename TValue>
    binary_deserializer & operator >> (std::map<TKey, TValue> & item)
    {
      auto count = this->read_number();
      for(decltype(count) i = 0; i < count; ++i){
        TKey key;
        TValue value;
        *this >> key >> value;
        item.emplace(std::move(key), std::move(value));
      }

      return *this;
    }

  private:
    const uint8_t * data_;
    size_t len_;
  };
}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
