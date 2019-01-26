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
#include "expected.h"

namespace vds {

  class binary_serializer
  {
  public:
    //bool
    expected<void> put(bool value);

    //1 byte
    expected<void> put(uint8_t value);

    //2 byte
    expected<void> put(uint16_t value);

    //4 bytes
    expected<void> put(uint32_t value);

    //8 bytes
    expected<void> put(uint64_t value);

    expected<void> write_number(uint64_t value);

    expected<void> put(const std::string & value);
    
    expected<void> push_data(const void * data, size_t size, bool serialize_size = true);
    
    expected<void> put(const const_data_buffer & data);

    const uint8_t * get_buffer() const {
      return this->data_.data();
    }

    const_data_buffer move_data() {
      return this->data_.move_data();
    }

    size_t size() const { return this->data_.size(); }

  private:
    resizable_data_buffer data_;
  };

  inline expected<void> serialize(binary_serializer & s, bool value) {
    return s.put(value);
  }

  //1 byte
  inline expected<void> serialize(binary_serializer & s, uint8_t value) {
    return s.put(value);
  }

  //2 byte
  inline expected<void> serialize(binary_serializer & s, uint16_t value) {
    return s.put(value);
  }

  //4 bytes
  inline expected<void> serialize(binary_serializer & s, uint32_t value) {
    return s.put(value);
  }

  //8 bytes
  inline expected<void> serialize(binary_serializer & s, uint64_t value) {
    return s.put(value);
  }


  //to avoid convert uint8_t * to std::string
  inline expected<void> serialize(binary_serializer & , const uint8_t * ) {
    throw std::runtime_error("Invalid error");
  }

  inline expected<void> serialize(binary_serializer & s, const std::string & value) {
    return s.put(value);
  }

  inline expected<void> serialize(binary_serializer & s, const const_data_buffer & data) {
    return s.put(data);
  }

  template <typename T>
  inline expected<void> serialize(binary_serializer & s, const std::list<T> & value)
  {
    CHECK_EXPECTED(s.write_number(value.size()));
    for (auto & p : value) {
      CHECK_EXPECTED(serialize(s, p));
    }

    return expected<void>();
  }

  template <typename T>
  inline expected<void> serialize(binary_serializer & s, const std::vector<T> & value)
  {
    CHECK_EXPECTED(s.write_number(value.size()));
    for (auto & p : value) {
      CHECK_EXPECTED(serialize(s, p));
    }

    return expected<void>();
  }

  template <typename T>
  inline expected<void> serialize(binary_serializer & s, const std::set<T> & value)
  {
    CHECK_EXPECTED(s.write_number(value.size()));
    for (auto & p : value) {
      CHECK_EXPECTED(serialize(s, p));
    }

    return expected<void>();
  }

  template <typename TKey, typename TValue>
  inline expected<void> serialize(binary_serializer & s, const std::map<TKey, TValue> & value)
  {
    CHECK_EXPECTED(s.write_number(value.size()));
    for (const auto & p : value) {
      CHECK_EXPECTED(serialize(s, p.first));
      CHECK_EXPECTED(serialize(s, p.second));
    }

    return expected<void>();
  }

  template <typename T>
  inline expected<void> serialize(binary_serializer & s, expected<T> && value) {
    if(value.has_error()) {
      return unexpected(std::move(value.error()));
    }

    return vds::serialize(s, value.value());    
  }

  class binary_deserializer
  {
  public:
    binary_deserializer(const const_data_buffer & data);
    binary_deserializer(const_data_buffer && data) = delete;
    binary_deserializer(const void * data, size_t len);
    
    //1 byte
    expected<void> get(bool & value);

    //1 byte
    expected<void> get(uint8_t & value);

    //2 byte
    expected<void> get(uint16_t & value);

    //4 byte
    expected<void> get(uint32_t & value);

    //8 byte
    expected<void> get(uint64_t & value);

    expected<void> get(std::string & value);
   
    expected<void> get(const_data_buffer & data);
    
    expected<uint64_t> read_number();

    const uint8_t * data() const { return this->data_; }
    size_t size() const { return this->len_; }
    
    expected<void> pop_data(void * data, size_t & size, bool serialize_size);
    expected<size_t> pop_data(void * data, size_t size);


  private:
    const uint8_t * data_;
    size_t len_;
  };

  //1 byte
  inline expected<void> deserialize(binary_deserializer & s, bool & value) {
    return s.get(value);
  }

  //1 byte
  inline expected<void> deserialize(binary_deserializer & s, uint8_t & value) {
    return s.get(value);
  }
  //2 byte
  inline expected<void> deserialize(binary_deserializer & s, uint16_t & value) {
    return s.get(value);
  }

  //4 byte
  inline expected<void> deserialize(binary_deserializer & s, uint32_t & value) {
    return s.get(value);
  }

  //8 byte
  inline expected<void> deserialize(binary_deserializer & s, uint64_t & value) {
    return s.get(value);
  }

  inline expected<void> deserialize(binary_deserializer & s, std::string & value) {
    return s.get(value);
  }

  inline expected<void> deserialize(binary_deserializer & s, const_data_buffer & data) {
    return s.get(data);
  }

  template <typename T>
  inline expected<void> deserialize(binary_deserializer & s, std::list<T> & value)
  {
    GET_EXPECTED(count, s.read_number());
    for (decltype(count) i = 0; i < count; ++i) {
      T item;
      CHECK_EXPECTED(vds::deserialize(s, item));
      value.push_back(item);
    }

    return expected<void>();
  }

  template <typename T>
  inline expected<void> deserialize(binary_deserializer & s, std::vector<T> & value)
  {
    GET_EXPECTED(count, s.read_number());
    for (decltype(count) i = 0; i < count; ++i) {
      T item;
      CHECK_EXPECTED(vds::deserialize(s, item));
      value.push_back(item);
    }

    return expected<void>();
  }

  template <typename T>
  inline expected<void> deserialize(binary_deserializer & s, std::set<T> & value)
  {
    GET_EXPECTED(count, s.read_number());
    for (decltype(count) i = 0; i < count; ++i) {
      T item;
      CHECK_EXPECTED(vds::deserialize(s, item));
      value.emplace(item);
    }

    return expected<void>();
  }

  template <typename TKey, typename TValue>
  inline expected<void> deserialize(binary_deserializer & s, std::map<TKey, TValue> & item)
  {
    GET_EXPECTED(count, s.read_number());
    for (decltype(count) i = 0; i < count; ++i) {
      TKey key;
      TValue value;
      CHECK_EXPECTED(vds::deserialize(s, key));
      CHECK_EXPECTED(vds::deserialize(s, value));

      item.emplace(std::move(key), std::move(value));
    }

    return expected<void>();
  }
  /////////////////////////////////////////////////////////////////////////
  // Helpers
  /////////////////////////////////////////////////////////////////////////
  template <typename... field_init_types>
  class _message_init_visitor;

  template <>
  class _message_init_visitor<> {
  public:
    _message_init_visitor(){
    }

    auto & operator ()() {
      return *this;
    }
  };

  template <typename field_init_type>
  class _message_init_visitor<field_init_type> {
  public:
    _message_init_visitor(field_init_type v)
      : v_(std::forward<field_init_type>(v)) {
    }

    template <typename field_type>
    auto & operator ()(field_type & field) {
      field = this->v_;
      return *this;
    }

  private:
    field_init_type v_;
  };

  template <typename first_field_init_type, typename... rest_fields_init_types>
  class _message_init_visitor<first_field_init_type, rest_fields_init_types...> : public _message_init_visitor<rest_fields_init_types...> {
    using base_class = _message_init_visitor<rest_fields_init_types...>;
  public:
    _message_init_visitor(first_field_init_type v, rest_fields_init_types... values)
      : base_class(std::forward<rest_fields_init_types>(values)...), v_(v) {
    }

    template <typename first_field_type, typename... rest_field_types>
    auto operator ()(first_field_type & first_field, rest_field_types &... rest_fields) {
      first_field = this->v_;
      return base_class::operator()(rest_fields...);
    }

    template <typename last_field_type>
    auto & operator ()(last_field_type & last_field) {
      last_field = this->v_;
      return *static_cast<base_class *>(this);
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
    _serialize_visitor & operator ()(field_types & ... fields) {
      serialize<field_types...>(fields...);
      return *this;
    }

    _serialize_visitor & operator ()(void) {
      return *this;
    }
    std::unique_ptr<std::exception>  & error() {
      return this->error_;
    }
  private:
    vds::binary_serializer & b_;
    std::unique_ptr<std::exception> error_;

    template <typename field_type>
    void serialize(field_type & field) {
      if (!this->error_) {
        auto result = vds::serialize(this->b_, field);
        if(result.has_error()) {
          this->error_ = std::move(result.error());
        }
      }
    }

    template <typename first_field_type, typename... rest_fields_types>
    void serialize(first_field_type & first_field, rest_fields_types &... rest_fields) {
      if (!this->error_) {
        auto result = vds::serialize(this->b_, first_field);
        if (result.has_error()) {
          this->error_ = std::move(result.error());
        }
      }
      serialize<rest_fields_types...>(rest_fields...);
    }
  };

  class _deserialize_visitor {
  public:
    _deserialize_visitor(vds::binary_deserializer & b)
      : b_(b) {
    }

    template <typename... field_types>
    _deserialize_visitor & operator ()(field_types &... fields) {
      deserialize<field_types...>(fields...);
      return *this;
    }

    _deserialize_visitor & operator ()(void) {
      return *this;
    }

    std::unique_ptr<std::exception>  & error() {
      return this->error_;
    }

  private:
    binary_deserializer & b_;
    std::unique_ptr<std::exception> error_;

    template <typename field_type>
    void deserialize(field_type & field) {
      if (!this->error_) {
        auto result = vds::deserialize(this->b_, field);
        if (result.has_error()) {
          this->error_ = std::move(result.error());
        }
      }
    }

    template <typename first_field_type, typename... rest_fields_types>
    void deserialize(first_field_type & first_field, rest_fields_types & ... rest_fields) {
      if (!this->error_) {
        auto result = vds::deserialize(this->b_, first_field);
        if (result.has_error()) {
          this->error_ = std::move(result.error());
        }
      }
      deserialize<rest_fields_types...>(rest_fields...);
    }
  };

  template<typename message_type, typename... init_field_types>
  inline expected<message_type> message_create(init_field_types &&... init_field_values)
  {
    message_type message;
    _message_init_visitor<init_field_types...> v(
        std::forward<init_field_types>(init_field_values)...);
    message.visit(v);

    return message;
  }

  template<typename message_type>
  inline expected<const_data_buffer> message_serialize(const message_type & message)
  {
    vds::binary_serializer b;
    _serialize_visitor bs(b);
    const_cast<typename std::remove_const<message_type>::type *>(&message)->visit(bs);
    if(bs.error()) {
      return vds::unexpected(std::move(bs.error()));
    }

    return b.move_data();
  }

  template<typename message_type>
  inline expected<message_type> message_deserialize(vds::binary_deserializer & d)
  {
    message_type message;
    _deserialize_visitor v(d);
    message.visit(v);
    if (v.error()) {
      return vds::unexpected(std::move(v.error()));
    }
    return message;
  }

}

#endif // __VDS_CORE_BINARY_SERIALIZE_H_
