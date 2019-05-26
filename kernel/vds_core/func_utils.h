#ifndef __VDS_CORE_FUNC_UTILS_H_
#define __VDS_CORE_FUNC_UTILS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include <functional>
#include <tuple>
#include <type_traits>

namespace vds {
  template <typename functor_type, typename functor_signature>
  struct _functor_info;

  template <typename functor_type, typename result, typename class_name, typename arg_type>
  struct _functor_info<functor_type, result(class_name::*)(arg_type)>
  {
    typedef result signature(arg_type);

    typedef typename std::remove_const<typename std::remove_reference<arg_type>::type>::type argument_type;
    typedef std::tuple<arg_type> arguments_tuple;
    typedef std::function<signature> function_type;
    typedef result result_type;

    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_type args) { f(args); });
    }
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type, typename result, typename class_name, typename arg_type>
  struct _functor_info<functor_type, result(class_name::*)(arg_type) const>
  {
    typedef result signature(arg_type);
    typedef typename std::remove_const<typename std::remove_reference<arg_type>::type>::type argument_type;
    typedef std::tuple<arg_type> arguments_tuple;
    typedef std::function<signature> function_type;
    typedef result result_type;

    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_type args) { f(args); });
    }

    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, result (class_name::*)(arg_types...)>
  {
    typedef result signature(arg_types...);
    
    typedef std::tuple<arg_types...> arguments_tuple;
    typedef std::function<signature> function_type;
    typedef result result_type;
    
    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types... args){ f(args...);});
    }
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };

  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  struct _functor_info<functor_type, result(class_name::*)(arg_types...) const>
  {
    typedef result signature(arg_types...);
    typedef std::tuple<arg_types...> arguments_tuple;
    typedef std::function<signature> function_type;
    typedef result result_type;
    
    static std::function<signature> to_function(functor_type & f)
    {
      return std::function<signature>([&f](arg_types ...args){ f(args...);});
    }
    
    static std::function<signature> to_function(functor_type && f)
    {
      return std::function<signature>(f);
    }
  };
  
  template <typename functor_type>
  struct functor_info : public _functor_info<functor_type, decltype(&functor_type::operator())>
  {};

  
  template<typename target_type, typename tuple_type, std::size_t current_num, std::size_t... nums>
  class _call_with : public _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>
  {
    using base_class = _call_with<target_type, tuple_type, current_num - 1, current_num - 1, nums...>;
  public:
    _call_with(const target_type & target, tuple_type && arguments)
    : base_class(target, std::forward<tuple_type>(arguments))
    {
    }
  };

  template<typename target_type, typename tuple_type, std::size_t... nums>
  class _call_with<target_type, tuple_type, 0, nums...>
  {
  public:
    _call_with(const target_type & target, tuple_type && arguments)
    {
      target(std::move(std::get<nums>(arguments))...);
    }
  };
  
  template<typename target_type, typename tuple_type>
  inline void call_with(const target_type & target, tuple_type && arguments)
  {
    _call_with<target_type, tuple_type, std::tuple_size<typename std::remove_reference<tuple_type>::type>::value>(
        target,
        std::forward<tuple_type>(arguments));
  }

  class func_utils
  {
  public:
    template <typename functor>
    static inline typename functor_info<functor>::function_type to_function(functor & f)
    {
      return functor_info<functor>::to_function(f);
    }
  };

  template <typename first_parameter_type, typename func_signature>
  class add_first_parameter;
  
  template <typename first_parameter_type, typename result_type, typename... argument_types>
  class add_first_parameter<first_parameter_type, result_type(argument_types...)>
  {
  public:
    typedef result_type type(first_parameter_type, argument_types...);
  };
  ///////////////////////////////////////
  template<typename... argument_types>
  class _set_values;

  template<>
  class _set_values<> {
  public:
    _set_values() {
    }

    void from() {
    }
  };

  template<typename first_parameter_type>
  class _set_values<first_parameter_type> {
  public:
    _set_values(first_parameter_type & value)
      :value_(value) {
    }

    void from(first_parameter_type && value) {
      this->value_ = value;
    }

  private:
    first_parameter_type & value_;
  };

  template<typename first_parameter_type, typename... argument_types>
  class _set_values<first_parameter_type, argument_types...> : protected _set_values<argument_types...> {
  public:
    _set_values(first_parameter_type & value, argument_types &... values)
      :_set_values<argument_types...>(values...),
      value_(value) {
      }

    void from(first_parameter_type && value, argument_types &&... values) {
      _set_values<argument_types...>::from(std::forward<argument_types...>(values...));
      this->value_ = value;
    }

  private:
    first_parameter_type & value_;
  };

  ///////////////////////////////////////
  /*
  template <typename lambda_type, typename lambda_signature>
  class _lambda_holder;
  
  template <typename lambda_type>
  class lambda_holder : public _lambda_holder<lambda_type, decltype(&lambda_type::operator())>
  {
  };
  
  template <typename functor_type, typename result, typename class_name, typename... arg_types>
  class _lambda_holder<functor_type, result (class_name::*)(arg_types...), std::enable_if<std::is_copy_constructible<functor_type>>
  {
  public:
    _lambda_holder(functor_type && f)
    : holder_(f)
    {
    }
    
    result operator()(arg_types... && args)
    {
      return this->holder_(std::forward<arg_types>(args)...);
    }
    
  protected:
    std::function<result(arg_types...)> holder_;
  };
  */

  template< class F >
  auto make_copyable_function(F&& f) {
	  using dF = std::decay_t<F>;
	  auto spf = std::make_shared<dF>(std::forward<F>(f));
	  return [spf](auto&&... args)->decltype(auto) {
		  return (*spf)(decltype(args)(args)...);
	  };
  }

  template<typename result_type, typename... arg_types>
  class lambda_holder_t
  {
  public:
	  lambda_holder_t() {
	  }

    lambda_holder_t(lambda_holder_t && origin)
    : holder_(std::move(origin.holder_)){
    }

	  template<typename F, class = typename std::enable_if<std::is_invocable_r_v<result_type, F, arg_types...> && !std::is_same<F, lambda_holder_t>::value>::type>
	  lambda_holder_t(F f)
		  : holder_(new holder<F>(std::move(f))) {
	  }

	  result_type operator ()(arg_types... args) const {
		  return (*this->holder_)(std::forward<arg_types>(args)...);
	  }

	  bool operator ! () const {
		  return !this->holder_;
	  }

	  explicit operator bool () const {
		  return this->holder_.get() != nullptr;
	  }

    lambda_holder_t & operator = (lambda_holder_t && original){
      this->holder_ = std::move(original.holder_);
      return *this;
    }

    void swap (lambda_holder_t & original) {
      auto tmp = std::move(original.holder_);
      original.holder_ = std::move(this->holder_);
      this->holder_ = std::move(tmp);
    }

  private:
	  class holder_base {
	  public:
		  virtual ~holder_base() {}
		  virtual result_type operator ()(arg_types... args) = 0;
	  };

	  template<typename F>
	  class holder : public holder_base {
	  public:
		  holder(F f)
			  : f_(std::move(f)) {
		  }

		  result_type operator ()(arg_types... args) override {
			  return this->f_(std::forward<arg_types>(args)...);
		  }

	  private:
		  F f_;
	  };

	  std::unique_ptr<holder_base> holder_;
  };
}

#endif//__VDS_CORE_FUNC_UTILS_H_
