#ifndef __VDS_CORE_EXPECTED_H_
#define __VDS_CORE_EXPECTED_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <exception>
#include <memory>
#include <stdexcept>
#include "vds_debug.h"

#if _WIN32
//not all control paths return a value
#pragma warning (error : 4715)

//not all control paths return a value
#pragma warning (error : 4033)

//discarding return value of function with 'nodiscard' attribute
#pragma warning (error : 4834)
#endif

namespace vds {

    class unexpected {
    public:
        unexpected(std::unique_ptr<std::exception> && error)
        : error_(std::move(error)){
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->error_;
        }

        std::unique_ptr<std::exception> & error() {
            return this->error_;
        }

    private:
        std::unique_ptr<std::exception> error_;
    };


    template <typename error_type, class... _Args>
    inline unexpected make_unexpected(_Args&&... __args){
        return unexpected(std::make_unique<error_type>(std::forward<_Args>(__args)...));
    }

    template <typename value_type>
    class [[nodiscard]] expected {
    public:
        template <typename... init_value_type>
        expected(init_value_type && ... value)
        : has_value_(true), value_(std::forward<init_value_type>(value)...){
        }

        expected(expected<value_type> && value) noexcept
          : has_value_(value.has_value()), value_(std::move(value.value_)), error_(std::move(value.error_)) {
        }

        expected(unexpected && error)
                : has_value_(false), error_(std::move(error.error())){
        }

        bool operator !() const {
            return !this->has_value_ && !this->error_;
        }

        bool has_value() const {
            return this->has_value_;
        }

        bool has_error() const {
            return !!this->error_;
        }

        const value_type & value() const {
            return this->value_;
        }

        value_type & value() {
            return this->value_;
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->error_;
        }

        std::unique_ptr<std::exception> & error() {
            return this->error_;
        }

      expected<value_type> & operator = (const expected<value_type> &) = delete;
      expected<value_type> & operator = (expected<value_type> && v) noexcept {
        this->has_value_ = v.has_value_;
        if(this->has_value_) {
          this->value_ = std::move(v.value_);          
        }
        this->error_ = std::move(v.error_);
        return *this;
      }


    private:
      bool has_value_;
      value_type value_;
      std::unique_ptr<std::exception> error_;
    };

    template <>
    class [[nodiscard]] expected<void> {
    public:
        expected()
                : has_value_(true){
        }

        expected(const expected<void> & origin) = delete;

        expected(expected<void> && origin) noexcept
          : has_value_(origin.has_value_), error_(std::move(origin.error_)) {
        }

        expected(unexpected && error)
                : has_value_(false), error_(std::move(error.error())){
        }

        bool has_value() const {
          return this->has_value_;
        }

        bool has_error() const {
          return !!this->error_;
        }

        void value() const {
          vds_assert(this->has_value_);
        }

        void value() {
          vds_assert(this->has_value_);
        }

        const std::unique_ptr<std::exception> & error() const {
            return this->error_;
        }

        std::unique_ptr<std::exception> & error() {
          return this->error_;
        }

        expected<void> & operator = (const expected<void> &) = delete;
        expected<void> & operator = (expected<void> && v) noexcept {
          this->has_value_ = v.has_value_;
          this->error_ = std::move(v.error_);
          return *this;
        }

    private:
      bool has_value_;
      std::unique_ptr<std::exception> error_;
    };
}

#define CHECK_EXPECTED_ERROR(v)\
  if((v).has_error()) {\
    return vds::unexpected(std::move((v).error()));\
  }

#define CHECK_EXPECTED(v)\
  {\
    auto __result { std::move(v) };\
    CHECK_EXPECTED_ERROR(__result);\
  }

#define GET_EXPECTED_VALUE(var, v)  \
  {\
    auto __result { std::move(v) };\
    if(__result.has_error()){\
      return vds::unexpected(std::move(__result.error()));\
    }\
    var = std::move(__result.value());\
  }

#define GET_EXPECTED(var, v) \
  typename std::remove_reference<decltype((v).value())>::type var;\
  GET_EXPECTED_VALUE(var, v);

#define WHILE_EXPECTED(exp) \
  for(;;) { \
    { \
      GET_EXPECTED(__while_cond, exp); \
      if (!__while_cond) { \
        break; \
      } \
    }
  
#define WHILE_EXPECTED_END() }


#define CHECK_EXPECTED_ERROR_ASYNC(v)\
  if((v).has_error()) {\
    co_return vds::unexpected(std::move((v).error()));\
  }

#define CHECK_EXPECTED_ASYNC(v) { auto __result { std::move(v) }; CHECK_EXPECTED_ERROR_ASYNC(__result); }

#define GET_EXPECTED_VALUE_ASYNC(var, v)  \
  {\
    auto __result { v };\
    if(__result.has_error()){\
      co_return vds::unexpected(std::move(__result.error()));\
    }\
    var = std::move(__result.value());\
  }
#define GET_EXPECTED_ASYNC(var, v) \
  typename std::remove_reference<decltype((v).value())>::type var; \
  GET_EXPECTED_VALUE_ASYNC(var, v);



#endif //__VDS_CORE_EXPECTED_H_
