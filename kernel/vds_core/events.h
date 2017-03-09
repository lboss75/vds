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
    event_handler();
    ~event_handler();
    
  private:
    friend class event_source<event_data_types...>;
    
    std::mutex sources_mutex_;
    std::list<event_source<event_data_types...>*> sources_;
  };
  
  template <typename... event_data_types>
  class event_source
  {
  public:
    event_source()
    {
    }
    
    ~event_source()
    {
      for(;;){
        this->subscribers_mutex_.lock();
        if(this->subscribers_.empty()){
          this->subscribers_mutex_.unlock();
          return;
        }
        
        auto& hadler = *this->subscribers_.begin();
        this->subscribers_mutex_.unlock();
        
        *this -= hadler;
      }
    }
    
    event_source & operator += (event_handler<event_data_types...> & handler)
    {
      handler.sources_mutex_.lock();
      handler.sources_.push_back(this);
      handler.sources_mutex_.unlock();
      
      this->subscribers_mutex_.lock();
      this->subscribers_.push_back(&handler);
      this->subscribers_mutex_.unlock();
    }
    
    event_source & operator -= (event_handler<event_data_types...> & handler)
    {
      handler.sources_mutex_.lock();
      handler.sources_.remove(this);
      handler.sources_mutex_.unlock();
      
      this->subscribers_mutex_.lock();
      this->subscribers_.remove(&handler);
      this->subscribers_mutex_.unlock();      
    }
    
    void operator ()(event_data_types&&... args)
    {
      std::map<event_handler<event_data_types...>*, bool> processed;
      for(;;){
        event_handler<event_data_types...>* handler = nullptr;
        
        this->subscribers_mutex_.lock();
        for(auto s : this->subscribers_){
          if(processed.end() == processed.find(s)){
            handler = s;
            break;
          }
        }
        this->subscribers_mutex_.unlock();
        
        if(nullptr == handler){
          break;
        }
        
        processed[handler] = true;
        (*handler)(std::forward<event_data_types>(args)...);
      }
    }

  private:
    std::mutex subscribers_mutex_;
    std::list<event_handler<event_data_types...>*> subscribers_;
  };
}


#endif//__VDS_CORE_EVENTS_H_
