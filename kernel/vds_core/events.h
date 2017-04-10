#ifndef __VDS_CORE_EVENTS_H_
#define __VDS_CORE_EVENTS_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  template <typename... event_data_types>
  class event_source;
  
  template <typename... event_data_types>
  class event_handler
  {
  public:
    event_handler(const std::function<void(event_data_types...)> & target);
    ~event_handler();

    void lock();
    void unlock();

    void operator()(event_data_types... args);
    
  private:
    friend class event_source<event_data_types...>;

    std::function<void(event_data_types...)> target_;

    std::mutex this_mutex_;

    std::mutex sources_mutex_;
    std::list<event_source<event_data_types...>*> sources_;
  };
  
  template <typename... event_data_types>
  class event_source
  {
  public:
    event_source();
    event_source(const std::function<void(event_source &)> & add_handler);
    ~event_source();
    
    event_source & operator += (event_handler<event_data_types...> & handler);
    
    event_source & operator -= (event_handler<event_data_types...> & handler);
    
    void operator ()(event_data_types... args);

  private:
    std::function<void(event_source &)> add_handler_;
    std::mutex subscribers_mutex_;
    std::list<event_handler<event_data_types...>*> subscribers_;
  };

  class event_switch
  {
  public:
    event_switch(unsigned int count);

    template <typename... event_data_types>
    void add_case(event_source<event_data_types...> & source, event_handler<event_data_types...> & handler);

  private:
    class iproxy
    {
    public:
      virtual ~iproxy() {}
    };

    template <typename... event_data_types>
    class proxy : public iproxy
    {
    public:
      proxy(event_switch * owner, event_source<event_data_types...> & source, event_handler<event_data_types...> & handler);
      ~proxy();

    private:
      event_switch * owner_;
      event_source<event_data_types...> & source_;
      event_handler<event_data_types...> & handler_;

      event_source<event_data_types...> proxy_source_;
      event_handler<event_data_types...> proxy_handler_;

      void proxy_handler(event_data_types... args);
    };

    std::mutex proxies_mutex_;
    unsigned int count_;

    enum state_enum
    {
      waiting,
      have_winner,
      ready_to_destroy
    };
    state_enum state_;
    std::list<std::unique_ptr<iproxy>> proxies_;

    bool try_to_win();
    void complete();
  };

  template<typename ...event_data_types>
  inline event_handler<event_data_types...>::event_handler(const std::function<void(event_data_types...)> & target)
    : target_(target)
  {
  }

  template<typename ...event_data_types>
  inline event_handler<event_data_types...>::~event_handler()
  {
    for (;;) {
      event_source<event_data_types...> * source;

      this->this_mutex_.lock();
      this->sources_mutex_.lock();
      if (this->sources_.empty()) {
        this->sources_mutex_.unlock();
        this->this_mutex_.unlock();
        return;
      }

      source = *this->sources_.begin();
      this->sources_.pop_front();
      this->sources_mutex_.unlock();
      this->this_mutex_.unlock();

      *source -= *this;
    }
  }
  template<typename ...event_data_types>
  inline void event_handler<event_data_types...>::lock()
  {
    this->this_mutex_.lock();
  }
  template<typename ...event_data_types>
  inline void event_handler<event_data_types...>::unlock()
  {
    this->this_mutex_.unlock();
  }
  template<typename ...event_data_types>
  inline void event_handler<event_data_types...>::operator()(event_data_types ...args)
  {
    this->target_(std::forward<event_data_types>(args)...);
  }
  template<typename ...event_data_types>
  inline event_source<event_data_types...>::event_source()
  {
  }
  template<typename ...event_data_types>
  inline event_source<event_data_types...>::event_source(const std::function<void(event_source &)> & add_handler)
    : add_handler_(add_handler)
  {
  }
  template<typename ...event_data_types>
  inline event_source<event_data_types...>::~event_source()
  {
    for (;;) {
      this->subscribers_mutex_.lock();
      if (this->subscribers_.empty()) {
        this->subscribers_mutex_.unlock();
        return;
      }

      auto hadler = *this->subscribers_.begin();
      this->subscribers_mutex_.unlock();

      *this -= *hadler;
    }
  }
  template<typename ...event_data_types>
  inline event_source<event_data_types...> & event_source<event_data_types...>::operator+=(event_handler<event_data_types...>& handler)
  {
    handler.sources_mutex_.lock();
    handler.sources_.push_back(this);
    handler.sources_mutex_.unlock();

    this->subscribers_mutex_.lock();
    this->subscribers_.push_back(&handler);
    this->subscribers_mutex_.unlock();

    if (this->add_handler_) {
      this->add_handler_(*this);
    }

    return *this;
  }
  template<typename ...event_data_types>
  inline event_source<event_data_types...> & event_source<event_data_types...>::operator-=(event_handler<event_data_types...>& handler)
  {
    handler.sources_mutex_.lock();
    handler.sources_.remove(this);
    handler.sources_mutex_.unlock();

    this->subscribers_mutex_.lock();
    this->subscribers_.remove(&handler);
    this->subscribers_mutex_.unlock();

    return *this;
  }
  template<typename ...event_data_types>
  inline void event_source<event_data_types...>::operator()(event_data_types... args)
  {
    std::map<event_handler<event_data_types...>*, bool> processed;

    this->subscribers_mutex_.lock();
    for (auto s : this->subscribers_) {
      if (processed.end() == processed.find(s)) {
        processed[s] = false;
      }
    }
    this->subscribers_mutex_.unlock();

    for (;;) {
      event_handler<event_data_types...>* handler = nullptr;

      this->subscribers_mutex_.lock();
      for (auto s : this->subscribers_) {
        auto p = processed.find(s);
        if (processed.end() != p
          && !p->second) {
          p->second = true;
          handler = s;
          handler->lock();
          break;
        }
      }
      this->subscribers_mutex_.unlock();

      if (nullptr == handler) {
        break;
      }

      (*handler)(std::forward<event_data_types>(args)...);
      handler->unlock();
    }
  }

  template<typename ...event_data_types>
  inline event_switch::proxy<event_data_types...>::proxy(event_switch * owner, event_source<event_data_types...>& source, event_handler<event_data_types...>& handler)
    : source_(source), handler_(handler), proxy_handler_(std::bind(&proxy::proxy_handler, this))
  {
    *this->source_ += *this->proxy_handler_;
    *this->proxy_source_ += handler_;
  }

  template<typename ...event_data_types>
  inline event_switch::proxy<event_data_types...>::~proxy()
  {
  }

  template<typename ...event_data_types>
  inline void event_switch::proxy<event_data_types...>::proxy_handler(event_data_types ...args)
  {
    if (this->owner_->try_win()) {
      this->proxy_source_(std::forward<event_data_types>(args)...);
      this->owner_->complete();
    }
  }

  inline vds::event_switch::event_switch(unsigned int count)
    : count_(count), state_(waiting)
  {
  }

  inline bool event_switch::try_to_win()
  {
    this->proxies_mutex_.lock();
    if (waiting != this->state_) {
      this->proxies_mutex_.unlock();
      return false;
    }

    this->state_ = have_winner;
    this->proxies_mutex_.unlock();
    return true;
  }

  inline void event_switch::complete()
  {
    bool need_to_destroy;
    this->proxies_mutex_.lock();
    this->state_ = ready_to_destroy;
    need_to_destroy = (0 == this->count_);
    this->proxies_mutex_.unlock();

    if (need_to_destroy) {
      delete this;
    }
  }

  template<typename ...event_data_types>
  inline void event_switch::add_case(event_source<event_data_types...>& source, event_handler<event_data_types...>& handler)
  {
    this->proxies_mutex_.lock();
    this->proxies_.push_back(new proxy<event_data_types...>(this, source, handler));
    this->count_--;
    if (0 == this->count_ && this->state_ == ready_to_destroy) {
      this->proxies_mutex_.unlock();
      delete this;
    }
    else {
      this->proxies_mutex_.unlock();
    }
  }

}


#endif//__VDS_CORE_EVENTS_H_
