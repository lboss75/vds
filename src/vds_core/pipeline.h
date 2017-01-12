#ifndef __VDS_CORE_PIPELINE_H_
#define __VDS_CORE_PIPELINE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "func_utils.h"

namespace vds {
  ////////////////////////////////////////////
  template <typename ... functor>
  class _pipeline_filters;
  
  template <typename functor>
  class _pipeline_filters<functor>
  {
  public:
    _pipeline_filters(const functor & target)
    : target_(target)
    {
    }
    
    
  private:
    functor target_;
  protected:
    
    class pipeline
    {
    public:
      pipeline(const error_handler_t & on_error, const _pipeline_filters & builder)
      : on_error_(on_error), target_(builder.target_)
      {
      }
      
      template <typename ...arguments>
      void
      operator()(const std::function<void(void)>& done, const error_handler_t & on_error, arguments ... args)
      {
        return this->target_(done, on_error, args...);
      }
      
    private:
      functor target_;
    protected:
      error_handler_t on_error_;
    };
  };
  
  
  template <typename filter, typename ...filters>
  class _pipeline_filters<filter, filters...>
    : public _pipeline_filters<filters...>
  {
  public:
    typedef _pipeline_filters<filters...> base;
    
    _pipeline_filters(const filter & f, filters... fs)
    : f_(f), base(fs...)
    {
    }
    
  private:
    filter f_;
    
  protected:
    class pipeline : protected base::pipeline
    {
    public:
      pipeline(const error_handler_t & on_error, const _pipeline_filters & builder)
      : contex_(builder.f_), base::pipeline(on_error, builder)
      {
      }
      
      template <typename ...arguments>
      void
      operator()(const std::function<void(void)> & done, const error_handler_t & on_error, arguments ... args)
      {
        this->contex_(
          done, on_error,
          *static_cast<typename base::pipeline *>(this), args ...);
      }
      
    private:
      typename filter::context contex_;
    };
  };
  
  //////////////////////////////////////////////////
  template <typename ... functor>
  class _pipeline_builder;
  
  
  template <typename first, typename ... functor>
  class _pipeline_builder<first, functor...>
  : public _pipeline_filters<functor...>
  {
  public:
    typedef _pipeline_builder<first, functor...> this_class;
    typedef _pipeline_filters<functor...> base;
    
    _pipeline_builder(const first & source, functor... filters)
    : source_(source), base(filters...)
    {
    }
    
    void
    start(const simple_done_handler_t & done, const error_handler_t & on_error)
    {
      auto pThis = this->generate_pipeline(done, on_error);
      pThis->start();
    }
    
  private:
    first source_;
    
    class pipeline : public base::pipeline, public std::enable_shared_from_this<pipeline>
    {
    public:
      pipeline(const simple_done_handler_t & done, const error_handler_t & on_error, const _pipeline_builder & builder)
      : done_(done), source_(builder.source_), base::pipeline(on_error, builder)
      {
      }

      ~pipeline()
      {
        this->done_();
      }
      
      void start()
      {
        this->source_(*this, this->on_error_);
      }
      
    private:
      simple_done_handler_t done_;
      first source_;
      
      template <typename ... arguments>
      void operator()(arguments ... args)
      {
        auto pThis = this->shared_from_this();
        (*static_cast<typename base::pipeline *>(this))([pThis]{
          pThis->source_(*pThis, pThis->on_error_);
        }, this->on_error_, args...);
      }
      
      template <typename ... arguments>
      operator std::function<void(arguments ... )> ()
      {
        auto pThis = this->shared_from_this();
        return [pThis](arguments ... args) {
          (*pThis)(args...);
        };
      }
    };
    
    std::shared_ptr<pipeline> generate_pipeline(const simple_done_handler_t & done, const error_handler_t & on_error) const
    {
      return std::shared_ptr<pipeline>(new pipeline(done, on_error, *this));
    }
  };

  
  template <typename ... functor>
  inline std::function<void(const simple_done_handler_t &, const error_handler_t & )>
  pipeline(functor ... functors)
  {
    std::shared_ptr<_pipeline_builder<functor...>> p(new _pipeline_builder<functor...>(functors...));
    return [p](const simple_done_handler_t & done, const error_handler_t & on_error) {
      p->start(done, on_error);
    };
  }
}

#endif // ! __VDS_CORE_PIPELINE_H_

