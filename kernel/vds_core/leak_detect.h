#ifndef __VDS_CORE_LEAK_DETECT_H_
#define __VDS_CORE_LEAK_DETECT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#ifdef _DEBUG

#include <string>
#include <set>
#include <map>
#include <functional>

namespace vds {

  class ileak_detect_helper;

  class leak_detect_resolver {
  public:
    void add(ileak_detect_helper * root_object){
      this->root_objects_.emplace(root_object);
    }

    std::string resolve();

    void add_reference(ileak_detect_helper * source, ileak_detect_helper * target){
      this->references_[source].targets_.emplace(target);
      if(this->references_.end() == this->references_.find(target)
          && this->unprocessed_.find(target) == this->unprocessed_.find(target)){
        this->unprocessed_.emplace(target);
      }
    }

  private:
    std::set<ileak_detect_helper *> root_objects_;
    std::set<ileak_detect_helper *> unprocessed_;

    struct target_info {
      target_info() : from_root_(false){}

      bool from_root_;
      std::set<ileak_detect_helper *> targets_;
    };
    std::map<ileak_detect_helper *, target_info> references_;

    void mark_as_root(ileak_detect_helper *obj);

    void process_leak(
        std::set<std::string> &result,
        const std::set<ileak_detect_helper *> &processed,
        ileak_detect_helper *target);
  };

  class leak_detect {
  public:

    static void add_object(ileak_detect_helper * helper);
    static void remove_object(ileak_detect_helper * helper);
  };

  class leak_detect_collector{
  public:
    leak_detect_collector(
        leak_detect_resolver * owner,
        ileak_detect_helper * source)
    : owner_(owner), source_(source){
    }

    template <typename T>
    void add(T & property){
      if(property){
        this->add_reference(&property->leak_detect_);
      }
    }


  private:
    friend class lambda_leak_detect_helper;

    leak_detect_resolver * owner_;
    ileak_detect_helper * source_;

    void add_reference(ileak_detect_helper * helper){
      owner_->add_reference(source_, helper);
    }
  };

  class ileak_detect_helper{
  public:
    virtual ~ileak_detect_helper() {
    }

    virtual std::string name() const = 0;
    virtual void dump_leaks(leak_detect_collector * collector) const = 0;
  };

  class leak_detect_helper : public ileak_detect_helper{
  public:
    leak_detect_helper() {
      leak_detect::add_object(this);
    }

    ~leak_detect_helper(){
      leak_detect::remove_object(this);
    }

    std::string name() const override {
      return this->name_;
    }

    void dump_leaks(leak_detect_collector * collector) const override {
      this->dump_callback_(collector);
    }

    std::string name_;
    std::function<void(leak_detect_collector *)> dump_callback_;
  };

  class lambda_leak_detect_helper : public ileak_detect_helper{
  public:
    template<typename... T>
    lambda_leak_detect_helper(const char * file_name, int line, T & ... properties)
    : name_(std::string(file_name) + "," + std::to_string(line)) {
      leak_detect::add_object(this);
      this->add(properties...);
    }
    ~lambda_leak_detect_helper(){
      leak_detect::remove_object(this);
    }

    std::string name() const override {
      return this->name_;
    }

    void dump_leaks(leak_detect_collector * collector) const override {
      for(auto p : this->properties_){
        collector->add_reference(p);
      }
    }
  private:
    std::string name_;
    std::set<ileak_detect_helper *> properties_;

    template<typename TFirst>
    void add(TFirst & property){
      this->properties_.emplace(&property->leak_detect_);
    }

    template<typename TFirst, typename... T>
    void add(TFirst & property, T & ... properties){
      this->properties_.emplace(&property->leak_detect_);
      this->add(properties...);
    }
  }
}
#else
namespace vds {

  class ileak_detect_helper {

  };

  class leak_detect_resolver {
  public:
    void add(ileak_detect_helper * root_object){
    }

    std::string resolve() {
      return std::string();
    }

    void add_reference(ileak_detect_helper * source, ileak_detect_helper * target){
    }
  };

  class leak_detect_collector{
  public:
    leak_detect_collector(
        leak_detect_resolver * owner,
        ileak_detect_helper * source) {
    }

    template <typename T>
    void add(T & property){
    }
  };

  class leak_detect_helper : public ileak_detect_helper{
  public:
    leak_detect_helper() {
    }

    ~leak_detect_helper(){
    }

    std::string name() const {
      return std::string();
    }

    void dump_leaks(leak_detect_collector * collector) const {
    }

    std::string name_;
    std::function<void(leak_detect_collector *)> dump_callback_;
  };
}

#endif


#endif //__VDS_CORE_LEAK_DETECT_H_
