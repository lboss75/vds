#ifndef __VDS_PROTOCOLS_ROUTE_MANAGER_H_
#define __VDS_PROTOCOLS_ROUTE_MANAGER_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <chrono>
#include "messages.h"

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
      const const_data_buffer & message_data);

    _route_manager * operator ->() const { return this->impl_; }
    
  private:
    _route_manager * const impl_;
  };
  
  class trace_point
  {
  public:
    trace_point(
      const guid & server_id,
      const std::string & address)
      : server_id_(server_id),
        address_(address)
    {
    }

    const guid & server_id() const { return this->server_id_; }
    const std::string & address() const { return this->address_; }

  private:
    guid server_id_;
    std::string address_;
  };

  class route_message
  {
  public:
    static const uint32_t message_type_id = (uint32_t)message_identification::route_message_message_id;

    route_message(const const_data_buffer & binary_form);
    void serialize(binary_serializer & b) const;
    
    route_message(
      const guid & message_id,
      const guid & target_server_id,
      uint32_t message_type_id,
      const const_data_buffer & message_data,
      const guid & server_id,
      const std::string & address)
    : message_id_(message_id),
      target_server_id_(target_server_id),
      message_type_id_(message_type_id),
      message_data_(message_data)
    {
      if (!address.empty()) {
        this->trace_route_.push_back(trace_point(server_id, address));
      }
    }

    route_message(
      const route_message & original_message,
      const guid & server_id,
      const std::string & address)
      : target_server_id_(original_message.target_server_id_),
      message_type_id_(original_message.message_type_id_),
      message_data_(original_message.message_data_),
      trace_route_(original_message.trace_route_)
    {
      if (!address.empty()) {
        this->trace_route_.push_back(trace_point(server_id, address));
      }
    }

    const guid & message_id() const { return this->message_id_;  }
    const guid & target_server_id() const { return this->target_server_id_; }
    uint32_t msg_type_id() const { return this->message_type_id_; }
    const const_data_buffer & message_data() const { return this->message_data_; }
    const std::list<trace_point> & trace_route() const { return this->trace_route_; }
      
  private:
    guid message_id_;
    guid target_server_id_;
    uint32_t message_type_id_;
    const_data_buffer message_data_;
    std::list<trace_point> trace_route_;
  };
}

#endif // __VDS_PROTOCOLS_ROUTE_MANAGER_H_
