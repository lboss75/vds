#ifndef __VDS_CORE_DEBUG_H_
#define __VDS_CORE_DEBUG_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <stdexcept>
#include <list>
#include <mutex>
#include <ostream>

namespace vds {
  class lifetime_check {
  public:
    lifetime_check() {
      this->signature_[0] = 'L';
      this->signature_[1] = 'I';
      this->signature_[2] = 'F';
      this->signature_[3] = 'E';
    }

    ~lifetime_check()
    {
      this->signature_[0] = 'D';
      this->signature_[1] = 'I';
      this->signature_[2] = 'E';
      this->signature_[3] = 'D';
    }

    void validate() const {
      if (this->signature_[0] != 'L'
        || this->signature_[1] != 'I'
        || this->signature_[2] != 'F'
        || this->signature_[3] != 'E') {
        throw std::runtime_error("Object disposed");
      }
    }

  private:
    char signature_[4];
  };

  class debug_task;
  class debug_task_type
  {
  public:
    debug_task_type(const char * )
    {

    }

    ~debug_task_type()
    {

    }

    void add(const std::string & item)
    {
      std::lock_guard<std::mutex> lock(this->items_mutex_);
      this->items_.push_back(item);
    }

    void remove(const std::string & item)
    {
      std::lock_guard<std::mutex> lock(this->items_mutex_);
      this->items_.remove(item);
    }

    void dump(std::ostream & s) const;

  private:
    mutable std::mutex items_mutex_;
    std::list<std::string> items_;
  };

  inline std::ostream & operator << (std::ostream & s, const debug_task_type & t)
  {
    t.dump(s);
    return s;
  }

  class debug_task
  {
  public:
    debug_task(debug_task_type & type, const char * name)
      : owner_(type), name_(name)
    {
      this->owner_.add(this->name_);
    }

    ~debug_task()
    {
      this->owner_.remove(this->name_);
    }

  private:
    debug_task_type & owner_;
    std::string name_;
  };

#define DECLARE_DEBUG_TASK(type)  extern vds::debug_task_type type;
#define DEFINE_DEBUG_TASK(type, name)  vds::debug_task_type type(name);
#define DEBUG_TASK(type, name)  debug_task __task(type, name);

#define START_DEBUG_TASK(type, name)  type.add(name);
#define FINISH_DEBUG_TASK(type, name)  type.remove(name);
}



#endif//__VDS_CORE_DEBUG_H_
