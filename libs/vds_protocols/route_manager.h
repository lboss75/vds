#ifndef __VDS_PROTOCOLS_ROUTE_MANAGER_H_
#define __VDS_PROTOCOLS_ROUTE_MANAGER_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>

namespace vds {
  class _route_manager;
  class route_manager
  {
  public:
    route_manager();
    ~route_manager();

    void add_route(
      const service_provider& sp,
      const guid & source_server_id,
      const guid & target_server_id,
      const std::string & address);
    
    struct network_route
    {
      guid target_server_id;
      std::string address;
    };
    
    void get_routes(
      const service_provider& sp,
      const guid & target_server_id,
      const std::function<bool(size_t metric, std::list<network_route> & routes)> & callback);
    
    void send_to(
      const service_provider & sp,
      const guid & server_id,
      uint32_t message_type_id,
      const std::function<const_data_buffer(void)> & get_binary,
      const std::function<std::string(void)> & get_json);

    _route_manager * operator ->() const { return this->impl_; }
    
  private:
    _route_manager * const impl_;
  };
  
  class route_message
  {
  public:
    route_message(
      const guid & target_server_id,
      const std::string & address,
      const std::chrono::steady_clock & last_access);
    
    const guid & target_server_id() const { return this->target_server_id_; }
    const std::string & address() const { return this->address_; }
    const std::chrono::steady_clock & last_access() const { return this->last_access_; }
      
  private:    
    guid target_server_id_;
    std::string address_;
    std::chrono::steady_clock last_access_;
  };
}

#endif // __VDS_PROTOCOLS_ROUTE_MANAGER_H_
