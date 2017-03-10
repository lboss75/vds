#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
  struct connection_message
  {
    bool is_broadcast;
    std::string from_address;
    std::list<std::string> to_address;
    std::string body;
  };
  

  class iconnection_channel
  {
  public:
    virtual void get_delivery_metrics(std::map<std::string, size_t> & metrics) = 0;
    virtual void send(const std::string & from_address, std::list<std::string> & to_address, const std::string &  body) = 0;
  };
  
  class _connection_manager;

  class connection_manager
  {
  public:
    connection_manager(
      const service_provider & sp,
      const std::string & from_address);
    ~connection_manager();
    
    void ready_to_get_messages(iconnection_channel * target);
    void remove_target(iconnection_channel * target);

    void send(const std::list<std::string> & to_address, const std::string &  body);

    template <typename message_type>
    void broadcast(const message_type & message)
    {
      this->get_peer_network().broadcast<message_type>(message);
    }

    peer_network & get_peer_network();

  private:
    _connection_manager * const impl_;
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
