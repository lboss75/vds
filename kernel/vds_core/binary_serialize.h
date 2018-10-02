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
  /////////////////////////////////////////////////////////////////////////
  // Helpers
  /////////////////////////////////////////////////////////////////////////
  template <typename... field_init_types>
  class _message_init_visitor;

  template <typename field_init_type>
  class _message_init_visitor<field_init_type> {
  public:
    _message_init_visitor(field_init_type && v)
      : v_(std::forward<field_init_type>(v)) {
    }

    template <typename field_type>
    void operator ()(field_type & field) {
      field = this->v_;
    }

  private:
    field_init_type v_;
  };

  template <typename first_field_init_type, typename... rest_fields_init_types>
  class _message_init_visitor<first_field_init_type, rest_fields_init_types...> : public _message_init_visitor<rest_fields_init_types...> {
    using base_class = _message_init_visitor<rest_fields_init_types...>;
  public:
    _message_init_visitor(first_field_init_type && v, rest_fields_init_types &&... values)
      : base_class(std::forward<rest_fields_init_types>(values)...), v_(v) {
    }

    template <typename first_field_type, typename... rest_field_types>
    void operator ()(first_field_type & first_field, rest_field_types &... rest_fields) {
      first_field = this->v_;
      base_class::operator()(rest_fields...);
    }

  private:
    first_field_init_type v_;
  };

  /////////////////////////////////////////////////////////////////////
  class _serialize_visitor {
  public:
    _serialize_visitor(vds::binary_serializer & b)
      : b_(b) {

    }

    template <typename... field_types>
    void operator ()(field_types & ... fields) {
      serialize<field_types...>(fields...);
    }

  private:
    vds::binary_serializer & b_;

    template <typename field_type>
    void serialize(field_type & field) {
      this->b_ << field;
    }

    template <typename first_field_type, typename... rest_fields_types>
    void serialize(first_field_type & first_field, rest_fields_types &... rest_fields) {
      this->b_ << first_field;
      serialize<rest_fields_types...>(rest_fields...);
    }
  };

  class _deserialize_visitor {
  public:
    _deserialize_visitor(vds::binary_deserializer & b)
      : b_(b) {
    }

    template <typename... field_types>
    void operator ()(field_types &... fields) {
      deserialize<field_types...>(fields...);
    }

  private:
    vds::binary_deserializer & b_;

    template <typename field_type>
    void deserialize(field_type & field) {
      this->b_ >> field;
    }

    template <typename first_field_type, typename... rest_fields_types>
    void deserialize(first_field_type & first_field, rest_fields_types & ... rest_fields) {
      this->b_ >> first_field;
      deserialize<rest_fields_types...>(rest_fields...);
    }
  };

  template<typename message_type, typename... init_field_types>
  inline message_type message_create(init_field_types &&... init_field_values)
  {
    message_type message;
    message.visit(_message_init_visitor<init_field_types...>(std::forward<init_field_types>(init_field_values)...));

    return message;
  }

  template<typename message_type, typename... init_field_types>
  inline const_data_buffer message_serialize(init_field_types &&... init_field_values)
  {
    message_type message;
    message.visit(
        _message_init_visitor<init_field_types...>(
            std::forward<init_field_types>(init_field_values)...));

    vds::binary_serializer b;
    message.visit(_serialize_visitor(b));

    return b.move_data();
  }

  template<typename message_type>
  inline message_type message_deserialize(vds::binary_deserializer & d)
  {
    message_type message;
    message.visit(_deserialize_visitor(d));
    return message;
  }

}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
