//#ifndef __VDS_CORE_std::future_H_
//#define __VDS_CORE_std::future_H_
//
///*
//Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
//All rights reserved
//*/
//#include "types.h"
//#include <memory>
//#include <atomic>
//#include <list>
//
//#include "func_utils.h"
//#include "result_or_error.h"
//#include "barrier.h"
//
//namespace vds {
//	template <typename... result_types>
//	class std::future;
//
//  template <typename... result_types>
//  class _std::future_base;
//
//  class _std::future_execute_token : public std::enable_shared_from_this<_std::future_execute_token>{
//  public:
//		class callback_base {
//		public:
//			virtual ~callback_base(){
//			}
//
//			virtual void invoke() = 0;
//		};
//
//		template<typename functor_type>
//		class callback_type : public callback_base{
//		public:
//			callback_type(functor_type && f)
//					: f_(std::move(f)){
//			}
//
//			virtual void invoke() {
//				this->f_();
//			}
//
//		private:
//			functor_type f_;
//		};
//
//    _std::future_execute_token()
//    : state_(state_type::BOF), callback_(nullptr) {
//    }
//
//    bool process(){
//      std::unique_lock<std::mutex> lock(this->state_mutex_);
//      switch (this->state_){
//        case state_type::BOF: {
//          this->state_ = state_type::DETACHED;
//          return false;
//        }
//        case state_type::ATTACHED: {
//          this->state_ = state_type::BOF;
//					auto callback = std::move(this->callback_);
//					this->callback_ = nullptr;
//          lock.unlock();
//
//          callback->invoke();
//					return true;
//        }
//        default:{
//          throw std::runtime_error("Login error");
//        }
//      }
//    }
//
//		template <typename callback_t>
//    void set_callback(callback_t && callback){
//      std::unique_lock<std::mutex> lock(this->state_mutex_);
//      switch (this->state_){
//        case state_type::BOF: {
//          this->callback_.reset(new callback_type<callback_t>(std::move(callback)));
//          this->state_ = state_type::ATTACHED;
//          break;
//        }
//
//        case state_type::DETACHED:{
//          this->state_ = state_type::BOF;
//          lock.unlock();
//
//          callback();
//
//					while(this->process());
//          break;
//        }
//
//        default:{
//          throw std::runtime_error("Login error");
//        }
//      }
//    }
//
//  private:
//    enum class state_type {
//      BOF,
//      ATTACHED,
//      DETACHED
//    };
//
//    std::mutex state_mutex_;
//    state_type  state_;
//		std::unique_ptr<callback_base> callback_;
//  };
//
//
//	template <typename... result_types>
//	class std::promise
//	{
//	public:
//		std::promise() = default;
//
//		std::promise(
//      const std::shared_ptr<_std::future_execute_token> & token,
//      _std::future_base<result_types...> * owner,
//      std::function<void(const std::shared_ptr<std::exception> & ex, result_types... results)> && callback);
//
//    std::promise(std::promise<result_types...> && origin)
//    : impl_(std::move(origin.impl_))
//    {
//    }
//
//    std::promise(const std::promise<result_types...> & origin)
//    : impl_(origin.impl_)
//    {
//    }
//    
//		void done(const result_types &... results) const;
//		void error(const std::shared_ptr<std::exception> & ex) const;
//
//		std::promise & operator =(const std::promise<result_types...> & origin)
//		{
//			this->impl_ = origin.impl_;
//			return *this;
//		}
//
//		bool operator !() const { return !this->impl_; }
//		operator bool () const { return this->impl_.get() != nullptr; }
//
//    void clear();
//
//    void add_owner(_std::future_base<result_types...> * owner);
//
//  private:
//    struct result_callback
//    {
//      result_callback(
//        const std::shared_ptr<_std::future_execute_token> & token,
//        _std::future_base<result_types...> * owner, 
//        std::function<void(const std::shared_ptr<std::exception> & ex, result_types... results)> && callback)
//      : done_(false), token_(token), callback_(std::move(callback))
//      {
//        this->owners_.push_back(owner);
//      }
//
//      ~result_callback();
//
//      void add_owner(_std::future_base<result_types...> * owner) {
//        this->owners_.push_back(owner);
//      }
//
//      bool done_;
//      std::shared_ptr<_std::future_execute_token> token_;
//      std::list<_std::future_base<result_types...> *> owners_;
//      std::function<void(const std::shared_ptr<std::exception> & ex, result_types... results)> callback_;
//    };
//    
//    mutable std::shared_ptr<result_callback> impl_;
//  };
//
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename argument_type>
//  struct _is_std::future_result
//  {
//	  static constexpr bool value = false;
//    static constexpr bool is_void_result = true;
//    typedef void task_type;
//  };
//
//  template <typename... result_types>
//  struct _is_std::future_result<const std::promise<result_types...> &>
//  {
//    static constexpr bool value = true;
//    static constexpr bool is_void_result = (0 == sizeof...(result_types));
//    typedef std::future<result_types...> task_type;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename result_type>
//  struct _std::future_result_helper
//  {
//	  typedef std::future<result_type> task_type;
//    static constexpr bool task_as_result = false;
//  };
//
//  template <>
//  struct _std::future_result_helper<void>
//  {
//	  typedef std::future<void> task_type;
//    static constexpr bool task_as_result = false;
//  };
//
//  template <typename... result_types>
//  struct _std::future_result_helper<std::future<result_types...>>
//  {
//	  typedef std::future<result_types...> task_type;
//    static constexpr bool task_as_result = true;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template<typename functor_signature>
//  struct _std::future_helper;
//
//  template <typename result_type, typename class_name, typename... arg_types>
//  struct _std::future_helper<result_type(class_name::*)(arg_types...)>
//  {
//    typedef std::function<result_type(arg_types...)> function_type;
//	  typedef typename _std::future_result_helper<result_type>::task_type task_type;
//	  static constexpr bool task_as_result = _std::future_result_helper<result_type>::task_as_result;
//	  static constexpr bool is_void_result = std::is_void<result_type>::value;
//  };
//
//  template <typename result_type, typename class_name, typename... arg_types>
//  struct _std::future_helper<result_type(class_name::*)(arg_types...) const>
//  {
//    typedef std::function<result_type(arg_types...)> function_type;
//	  typedef typename _std::future_result_helper<result_type>::task_type task_type;
//	  static constexpr bool task_as_result = _std::future_result_helper<result_type>::task_as_result;
//	  static constexpr bool is_void_result = std::is_void<result_type>::value;
//  };
//
//  /////////////////////////////////////////////////////////////////////////////////
//  template<typename functor_signature>
//  struct _std::future_functor_helper
//  {
//	  static constexpr bool is_async_callback = false;
//	  static constexpr bool task_as_result = _std::future_helper<functor_signature>::task_as_result;
//	  static constexpr bool is_void_result = _std::future_helper<functor_signature>::is_void_result;
//	  typedef typename _std::future_helper<functor_signature>::task_type task_type;
//	  typedef typename _std::future_helper<functor_signature>::function_type function_type;
//  };
//
//  template <typename class_name, typename first_argument_type, typename... arg_types>
//  struct _std::future_functor_helper<void(class_name::*)(first_argument_type, arg_types...) const>
//  {
//	  static constexpr bool is_async_callback = _is_std::future_result<first_argument_type>::value;
//	  static constexpr bool task_as_result = false;
//    static constexpr bool is_void_result = 
//      !_is_std::future_result<first_argument_type>::value
//      || _is_std::future_result<first_argument_type>::is_void_result;
//
//	  typedef typename std::conditional<
//		  _is_std::future_result<first_argument_type>::value,
//		  typename _is_std::future_result<first_argument_type>::task_type,
//		  std::future<void> >::type task_type;
//	  typedef std::function<void(first_argument_type, arg_types...)> function_type;
//  };
//
//  template <typename class_name, typename first_argument_type, typename... arg_types>
//  struct _std::future_functor_helper<void(class_name::*)(first_argument_type, arg_types...)>
//  {
//	  static constexpr bool is_async_callback = _is_std::future_result<first_argument_type>::value;
//	  static constexpr bool task_as_result = false;
//    static constexpr bool is_void_result =
//      !_is_std::future_result<first_argument_type>::value
//      || _is_std::future_result<first_argument_type>::is_void_result;
//    typedef typename std::conditional<
//		  _is_std::future_result<first_argument_type>::value,
//		  typename _is_std::future_result<first_argument_type>::task_type,
//		  std::future<void> >::type task_type;
//	  typedef std::function<void(first_argument_type, arg_types...)> function_type;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename... result_types>
//  class std::future
//  {
//  public:
//	  template<typename functor_type>
//	  std::future(functor_type && f,
//		  typename std::enable_if<_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);
//
//	  template<typename functor_type>
//	  std::future(
//		  functor_type && f,
//		  typename std::enable_if<!_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);
//
//    std::future(const std::future<result_types...> & origin) = delete;
//    std::future(std::future<result_types...> && origin);
//	  std::future(std::shared_ptr<std::exception> && error);
//
//	  std::future(const std::shared_ptr<std::exception> & error);
//    
//	  template<typename parent_task_type, typename functor_type>
//	  std::future(
//		  parent_task_type && parent,
//		  functor_type && f);
//    
//	  ~std::future();
//
//	  template<typename functor_type>
//	  void execute(functor_type && done_callback);
//    
//	  template<typename functor_type>
//	  void operator()(
//				const std::shared_ptr<_std::future_execute_token> & token,
//				functor_type && done_callback);
//
//	  std::future & operator = (std::future<result_types...> && origin);
//    std::future & operator = (const std::future<result_types...> & origin) = delete;
//
//
//	  template<typename functor_type>
//	  auto then(functor_type && f)
//	  -> typename _std::future_functor_helper<decltype(&functor_type::operator())>::task_type;
//
//	  template<typename functor_type, typename done_type>
//	  void execute_with(
//    const std::shared_ptr<_std::future_execute_token> & token,
//		functor_type & f,
//		done_type && done,
//		typename std::enable_if<_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type * = nullptr);
//
//	  template<typename functor_type, typename done_type>
//	  void execute_with(
//      const std::shared_ptr<_std::future_execute_token> & token,
//		  functor_type & f,
//		  done_type && done,
//		  typename std::enable_if<
//		  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//		  && _std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result>::type * = nullptr);
//    
//	  template<typename functor_type, typename done_type>
//	  void execute_with(
//      const std::shared_ptr<_std::future_execute_token> & token,
//		  functor_type & f,
//		  done_type && done,
//		  typename std::enable_if<
//		  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//		  && !_std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result
//		  && _std::future_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type * = nullptr);
//    
//	  template<typename functor_type, typename done_type>
//	  void execute_with(
//      const std::shared_ptr<_std::future_execute_token> & token,
//		  functor_type & f,
//		  done_type && done,
//		  typename std::enable_if<
//		  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//		  && !_std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result
//		  && !_std::future_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type * = nullptr);
//
//    void wait(result_types & ... values) {
//      barrier b;
//      std::shared_ptr<std::exception> error;
//      this->execute([&](const std::shared_ptr<std::exception> & ex, result_types ... args) {
//        if(ex) {
//          error = ex;
//        }
//        else {
//          _set_values<result_types...>(values...).from(args...);
//        }
//        b.set();
//      });
//      b.wait();
//      if(error) {
//        throw std::runtime_error(error->what());
//      }
//    }
//
//    void no_wait() {
//      this->execute([&](const std::shared_ptr<std::exception> & ex, result_types ... args) {
//        if (ex) {
//        }
//      });
//    }
//
//    static std::future<result_types...> result(result_types... values);
//		static std::future<result_types...> empty();
//  private:
//    std::future() = delete;
//	  _std::future_base<result_types...> * impl_;
//    
//	  std::future(_std::future_base<result_types...> * impl);
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename... result_types>
//  class _std::future_base
//  {
//  public:
//	  virtual ~_std::future_base() {}
//
//	  virtual void execute(
//        const std::shared_ptr<_std::future_execute_token> & token,
//        std::promise<result_types...> && done) = 0;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename functor_type, typename... result_types>
//  class _std::future_async_callback : public _std::future_base<result_types...>
//  {
//  public:
//	  _std::future_async_callback(functor_type && f)
//		  : f_(std::move(f))
//	  {
//	  }
//
//	  void execute(
//        const std::shared_ptr<_std::future_execute_token> & token,
//        std::promise<result_types...> && done) override
//	  {
//      try {
//        this->f_(done);
//      }
//      catch (const std::exception & ex) {
//        done.error(std::make_shared<std::runtime_error>(ex.what()));
//      }
//      catch (...) {
//        done.error(std::make_shared<std::runtime_error>("Unexpected error"));
//      }
//	  }
//
//  private:
//	  functor_type f_;
//  };
//
//  template <typename functor_type, typename... result_types>
//  class _std::future_sync_callback : public _std::future_base<result_types...>
//  {
//  public:
//	  _std::future_sync_callback(functor_type && f)
//		  : f_(f)
//	  {
//	  }
//
//	  void execute(const std::shared_ptr<_std::future_execute_token> & token, std::promise<result_types...> && done) override
//	  {
//		  try {
//			  done.done(this->f_());
//		  }
//		  catch (const std::exception & ex) {
//			  done.error(std::make_shared<std::runtime_error>(ex.what()));
//		  }
//		  catch (...) {
//			  done.error(std::make_shared<std::runtime_error>("Unexpected error"));
//		  }
//	  }
//
//  private:
//	  functor_type f_;
//  };
//
//  template <typename functor_type>
//  class _std::future_sync_callback<functor_type> : public _std::future_base<>
//  {
//  public:
//	  _std::future_sync_callback(functor_type && f)
//		  : f_(f)
//	  {
//	  }
//
//	  void execute(
//				const std::shared_ptr<_std::future_execute_token> & token,
//				std::promise<> && done) override
//	  {
//      try {
//        this->f_();
//      }
//      catch (const std::exception & ex) {
//        done.error(std::make_shared<std::runtime_error>(ex.what()));
//        return;
//      }
//      catch (...) {
//        done.error(std::make_shared<std::runtime_error>("Unexpected error"));
//        return;
//      }
//  		done.done();
//	  }
//
//  private:
//	  functor_type f_;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename... result_types>
//  class _std::future_error : public _std::future_base<result_types...>
//  {
//  public:
//	  _std::future_error(std::shared_ptr<std::exception> && error)
//		  : error_(std::move(error))
//	  {
//	  }
//	  
//	  _std::future_error(const std::shared_ptr<std::exception> & error)
//		  : error_(error)
//	  {
//	  }
//
//	  void execute(
//				const std::shared_ptr<_std::future_execute_token> & token,
//				std::promise<result_types...> && done) override {
//			done.error(this->error_);
//	  }
//
//  private:
//	  std::shared_ptr<std::exception> error_;
//  };
//	/////////////////////////////////////////////////////////////////////////////////
//	template <typename... result_types>
//	class _std::future_value : public _std::future_base<result_types...>
//	{
//	public:
//		_std::future_value(result_types && ... values)
//				: values_(std::make_tuple(std::forward<result_types>(values)...))
//		{
//		}
//
//		void execute(
//				const std::shared_ptr<_std::future_execute_token> & token,
//				std::promise<result_types...> && done) override
//		{
//			call_with([d = std::move(done)](result_types &&... args) {
//				d.done(std::forward<result_types>(args)...);
//			}, this->values_);
//		}
//
//	private:
//		std::tuple<result_types...> values_;
//	};
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename... result_types>
//  class _std::future_empty : public _std::future_base<result_types...>
//  {
//  public:
//	  _std::future_empty()
//	  {
//	  }
//
//	  void execute(
//        const std::shared_ptr<_std::future_execute_token> & /*token*/,
//        std::promise<result_types...> && done) override
//	  {
//      done.done(result_types()...);
//	  }
//  };
//
//  /////////////////////////////////////////////////////////////////////////////////
//  template <typename parent_task_type, typename functor_type, typename... result_types>
//  class _std::future_joined_callback : public _std::future_base<result_types...>
//  {
//  public:
//	  _std::future_joined_callback(parent_task_type && parent, functor_type && f)
//		  : parent_(std::move(parent)), f_(std::move(f))
//	  {
//	  }
//
//	  void execute(const std::shared_ptr<_std::future_execute_token> & token, std::promise<result_types...> && done) override
//	  {
//		  this->parent_.execute_with(token, this->f_, std::move(done));
//	  }
//
//  private:
//	  parent_task_type parent_;
//	  functor_type f_;
//  };
//  /////////////////////////////////////////////////////////////////////////////////
//  template<typename ...result_types>
//  template<typename functor_type>
//  inline std::future<result_types...>::std::future(
//	  functor_type && f,
//	  typename std::enable_if<_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
//  : impl_(new _std::future_async_callback<functor_type, result_types...>(std::move(f)))
//  {
//  }
//    
//  template<typename ...result_types>
//  template<typename functor_type>
//  inline std::future<result_types...>::std::future(
//	  functor_type && f,
//	  typename std::enable_if<!_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
//  : impl_(new _std::future_sync_callback<functor_type, result_types...>(std::move(f)))
//  {
//  }
//
//  template<typename ...result_types>
//  inline std::future<result_types...>::std::future(std::future<result_types...>&& origin)
//  : impl_(origin.impl_)
//  {
//	  origin.impl_ = nullptr;
//  }
//
//	template<typename ...result_types>
//	inline std::future<result_types...> std::future<result_types...>::result(result_types ... values)
//	{
//		return std::future<result_types...>(
//				new _std::future_value<result_types...>(std::forward<result_types>(values)...));
//	}
//
//  template<typename ...result_types>
//  inline std::future<result_types...>::std::future(std::shared_ptr<std::exception> && error)
//  : impl_(new _std::future_error<result_types...>(std::move(error)))
//  {
//  }
//  	  
//  template<typename ...result_types>
//  inline std::future<result_types...>::std::future(const std::shared_ptr<std::exception> & error)
//  : impl_(new _std::future_error<result_types...>(error))
//  {
//  }
//
//  template<typename ...result_types>
//  template<typename parent_task_type, typename functor_type>
//  inline std::future<result_types...>::std::future(
//	  parent_task_type && parent,
//	  functor_type && f)
//  : impl_(new _std::future_joined_callback<parent_task_type, functor_type, result_types...>(
//    std::forward<parent_task_type>(parent),
//    std::forward<functor_type>(f)))
//  {
//  }
//
//  template<typename ...result_types>
//  inline std::future<result_types...>::std::future(_std::future_base<result_types...> * impl)
//  : impl_(impl)
//  {
//  }
//
//  template<typename ...result_types>
//  inline std::future<result_types...>::~std::future()
//  {
//#if defined(DEBUG) || defined(_DEBUG)
//#ifdef _WIN32
//#pragma warning(disable: 4297)
//#endif
//	  if(nullptr != this->impl_){
//      throw std::runtime_error("Task without execute");
//    }
//#ifdef _WIN32
//#pragma warning(default: 4297)
//#endif
//#endif//DEBUG
//
//	  delete this->impl_;
//  }
//
//  template<typename ...result_types>
//  template<typename functor_type>
//  inline void std::future<result_types...>::execute(functor_type && done_callback)
//  {
//    auto token = std::make_shared<_std::future_execute_token>();
//    auto impl = this->impl_;
//    this->impl_ = nullptr;
//    impl->execute(
//      token,
//		  std::promise<result_types...>(
//        token,
//        impl,
//			  std::move(done_callback)));
//
//    while(token->process());
//  }
//  
//  template<typename ...result_types>
//  template<typename functor_type>
//  inline void std::future<result_types...>::operator()(
//		const std::shared_ptr<_std::future_execute_token> & token,
//	  functor_type && done_callback)
//  {
//		auto impl = this->impl_;
//		this->impl_ = nullptr;
//    done_callback.add_owner(impl);
//	  impl->execute(token, std::move(done_callback));
//  }
//
//  template<typename ...result_types>
//  template<typename functor_type>
//  inline auto std::future<result_types...>::then(functor_type && f)
//	  -> typename _std::future_functor_helper<decltype(&functor_type::operator())>::task_type
//  {
//	  return typename _std::future_functor_helper<decltype(&functor_type::operator())>::task_type(
//		  std::move(*this),
//      std::forward<functor_type>(f));
//  }
//
//  template<typename ...result_types>
//  template<typename functor_type, typename done_type>
//  inline void std::future<result_types...>::execute_with(
//  const std::shared_ptr<_std::future_execute_token> & token,
//	functor_type & f,
//	done_type && done,
//	typename std::enable_if<_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback>::type *)
//  {
//		auto impl = this->impl_;
//		this->impl_ = nullptr;
//	  impl->execute(
//		  std::promise<result_types...>(
//        impl,
//			  [&f, d = std::move(done), token](const std::shared_ptr<std::exception> & ex, result_types... result) {
//          if(!ex){
//            f(d, std::forward<result_types>(result)...);
//          } else {
//            d.error(ex);
//          }
//			}));
//  }
//
//  template<typename ...result_types>
//  template<typename functor_type, typename done_type>
//  inline void std::future<result_types...>::execute_with(
//    const std::shared_ptr<_std::future_execute_token> & token,
//	  functor_type & f,
//	  done_type && done,
//	  typename std::enable_if<
//	  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//	  && _std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result>::type *)
//  {
//		auto impl = this->impl_;
//		this->impl_ = nullptr;
//	  impl->execute(
//			token,
//		  std::promise<result_types...>(token, impl, [token, &f, d = std::move(done)](const std::shared_ptr<std::exception> & ex, result_types... result) mutable {
//        if(!ex){
//			try {
//				auto t = f(std::forward<result_types>(result)...);
//#ifdef __clang__
//        t.template operator()<done_type>(token, std::move(d));
//#else
//        t.operator()<done_type>(token, std::move(d));
//#endif
//			}
//			catch (const std::exception & ex) {
//				d.error(std::make_shared<std::runtime_error>(ex.what()));
//			}
//        } else {
//          d.error(ex);
//        }
//	  }));
//  }
//
//  template<typename ...result_types>
//  template<typename functor_type, typename done_type>
//  inline void std::future<result_types...>::execute_with(
//    const std::shared_ptr<_std::future_execute_token> & token,
//	  functor_type & f,
//	  done_type && done,
//	  typename std::enable_if<
//	  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//	  && !_std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result
//	  && _std::future_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
//  {
//		auto impl = this->impl_;
//		this->impl_ = nullptr;
//	  impl->execute(
//			token,
//		  std::promise<result_types...>(
//        token,
//        impl,
//			  [token, &f, done](const std::shared_ptr<std::exception> & ex, result_types... result) {
//          if(!ex){
//            try {
//              f(std::forward<result_types>(result)...);
//            } catch(const std::exception & ex){
//              done.error(std::make_shared<std::runtime_error>(ex.what()));
//              return;
//            }
//            done.done();
//          } else {
//            done.error(ex);
//          }
//	  }));
//  }
//  
//  template<typename ...result_types>
//  template<typename functor_type, typename done_type>
//  inline void std::future<result_types...>::execute_with(
//    const std::shared_ptr<_std::future_execute_token> & token,
//	  functor_type & f,
//	  done_type && done,
//	  typename std::enable_if<
//	  !_std::future_functor_helper<decltype(&functor_type::operator())>::is_async_callback
//	  && !_std::future_functor_helper<decltype(&functor_type::operator())>::task_as_result
//	  && !_std::future_functor_helper<decltype(&functor_type::operator())>::is_void_result>::type *)
//  {
//		auto impl = this->impl_;
//		this->impl_ = nullptr;
//	  impl->execute(token, std::promise<result_types...>(token, impl, [&f, done, token](
//          const std::shared_ptr<std::exception> & ex,
//          result_types... result) {
//          if(!ex){
//            try {
//              auto t = f(std::forward<result_types>(result)...);
//              done.done(t);
//            } catch(const std::exception & ex){
//              done.error(std::make_shared<std::runtime_error>(ex.what()));
//            }        
//          } else {
//            done.error(ex);
//          }
//        }));
//  }
//  
//  template<typename ...result_types>
//  inline std::future<result_types...> std::future<result_types...>::empty()
//  {
//    return std::future<result_types...>(static_cast<_std::future_base<result_types...> *>(new _std::future_empty<result_types...>()));
//  }
//
//	template<typename ...result_types>
//	inline std::future<result_types...> &std::future<result_types...>::operator=(std::future<result_types...> &&origin) {
//		delete this->impl_;
//		this->impl_ = origin.impl_;
//		origin.impl_ = nullptr;
//		return *this;
//	}
//
//	/////////////////////////////////////////////////////////////////////////////////
//  template<typename ...result_types>
//  inline std::promise<result_types...>::std::promise(
//    const std::shared_ptr<_std::future_execute_token> & token,
//    _std::future_base<result_types...> * owner,
//	  std::function<void(const std::shared_ptr<std::exception> & ex, result_types...results)> && callback)
//	: impl_(new result_callback(token, owner, std::move(callback)))
//  {
//  }
//
//  template<typename ...result_types>
//  inline void std::promise<result_types...>::done(const result_types & ...results) const
//  {
//    auto impl = std::move(this->impl_);
//    impl->token_->set_callback([impl, results...](){
//      impl->done_ = true;
//      impl->callback_(std::shared_ptr<std::exception>(), results...);
//    });
//  }
//
//  template<typename ...result_types>
//  inline void std::promise<result_types...>::error(const std::shared_ptr<std::exception>& ex) const
//  {
//    auto impl = std::move(this->impl_);
//    impl->token_->set_callback([impl, ex]() {
//      impl->done_ = true;
//      impl->callback_(ex, typename std::remove_reference<result_types>::type()...);
//    });
//  }
//
//  template<typename ...result_types>
//  inline void std::promise<result_types...>::clear() {
//		this->impl_.reset();
//  }
//
//  template <typename ... result_types>
//  void std::promise<result_types...>::add_owner(_std::future_base<result_types...>* owner) {
//    this->impl_->add_owner(owner);
//  }
//
//  template <typename ... result_types>
//  std::promise<result_types...>::result_callback::~result_callback() {
//    for(auto owner : this->owners_) {
//      delete owner;
//    }
//#if defined(DEBUG) || defined(_DEBUG)
//#ifdef _WIN32
//#pragma warning(disable: 4297)
//#endif
//      if (!this->done_) {
//        throw std::runtime_error("Task without execute");
//      }
//#ifdef _WIN32
//#pragma warning(default: 4297)
//#endif
//#endif//DEBUG
//  }
//
//  /////////////////////////////////////////////////////////////////////////////////
//  class _async_series
//  {
//  public:
//    _async_series(
//      const std::promise<> & result,
//      size_t count)
//    : result_(result), count_(count)
//    {
//    }
//    
//    void run(std::list<std::future<void>> && args)
//    {
//      for (auto && arg : args) {
//        this->add(std::move(arg));
//      }
//    }
//    
//    _async_series & add(std::future<void> && arg)
//    {
//      arg.execute(
//        [this](const std::shared_ptr<std::exception> & ex) {
//          if(ex && !this->error_) {
//              this->error_ = ex;
//          }
//          
//          if (0 == --this->count_) {
//            if (this->error_) {
//              this->result_.error(this->error_);
//            }
//            else {
//              this->result_.done();
//            }            
//            delete this;
//          }
//        });
//      return *this;
//    }
//    
//  private:
//    std::promise<> result_;
//    std::atomic_size_t count_;
//    std::shared_ptr<std::exception> error_;
//  };
//  
//  template <typename... task_types>
//  inline void _empty_fake(task_types... args)
//  {
//  }
//  
//  template <typename task_type>
//  inline void _add_to_async_series(std::list<std::future<void>> & target, task_type && first)
//  {
//    target.push_back(std::move(first));
//  }
//  
//  template <typename task_type, typename... task_types>
//  inline void _add_to_async_series(std::list<std::future<void>> & target, task_type && first, task_types &&... args)
//  {
//    target.push_back(std::move(first));
//    _add_to_async_series(target, std::move(args)...);
//  }
//
//
//  template <typename... task_types>
//  inline std::future<void> async_series(task_types &&... args)
//  {
//    auto steps = new std::list<std::future<void>>();
//    _add_to_async_series(*steps, std::move(args)...);
//    
//    return [steps](const std::promise<> & result){
//        auto runner = new _async_series(result, steps->size());
//        runner->run(std::move(*steps));
//        delete steps;
//      };
//  }
//}
//
//#endif // __VDS_CORE_std::future_H_
// 
