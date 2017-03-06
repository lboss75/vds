#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class connection_manager;
  
  class _connection_manager
  {
  public:
    _connection_manager(
      const service_provider & sp,
      connection_manager * owner,
      const std::string & from_address);
    ~_connection_manager();

    void ready_to_get_messages(iconnection_channel * target);
    void send(const std::list<std::string> & to_address, const std::string &  body);
    void broadcast(const std::string &  body);

  private:
    service_provider sp_;
    connection_manager * const owner_;
    std::string from_address_;
    
    std::mutex connection_channels_mutex_;
    std::list<iconnection_channel *> connection_channels_;
    
    std::mutex messages_mutex_;
    std::list<connection_message> messages_;
    
    void work_thread();
  };
}

#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_P_H_
