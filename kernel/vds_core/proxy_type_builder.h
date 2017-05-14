#ifndef __VDS_CORE_PROXY_TYPE_BUILDER_H_
#define __VDS_CORE_PROXY_TYPE_BUILDER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include "types.h"

namespace vds {
  template<typename proxy_interface, template<typename> typename proxy_type>
  class proxy_type_builder
  {
  public:
    
    template <typename target_type>
    proxy_interface & get(target_type & target)
    {
      auto id = types::get_type_id<target_type>();
      auto p = this->proxies_.find(id);
      if(this->proxies_.end() == p){
        auto builder = new proxy_builder<target_type>();
        this->proxies_[id].reset(builder);
        return builder->get(target);
      }
      else {
        return ((proxy_builder<target_type> *)p->second.get())->get(target);
      }
    }
    
  private:
    class iproxy_builder
    {
    public:
      virtual ~iproxy_builder(){}
    };
    
    template <typename target_type>
    class proxy_builder : public iproxy_builder
    {
    public:
      proxy_interface & get(target_type & target)
      {
        auto p = this->targets_.find(&target);
        if(this->targets_.end() == p){
          auto result = new proxy_type<target_type>(target);
          this->targets_[&target].reset(result);
          return *result;
        }
        else {
          return *p->second.get();
        }
      }
    private:
      std::map<const target_type *, std::unique_ptr<proxy_interface>> targets_;
    };
    
    std::map<size_t, std::unique_ptr<iproxy_builder>> proxies_;
  };
}

#endif // __VDS_CORE_PROXY_TYPE_BUILDER_H_
