#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  
  struct connection_message
  {
    std::string from_address;
    std::list<std::string> to_address;
    std::string body;
  };
  

  class iconnection_channel
  {
  public:
    struct target_metric
    {
      iconnection_channel * channel;
      size_t metric;
    };
    
    virtual void update_target_metrics(std::map<std::string, target_metric> & metrics) = 0;
  };
  
  class _connection_manager;

  class connection_manager
  {
  public:
    connection_manager(const service_provider & sp);
    ~connection_manager();
    
    void ready_to_get_messages(iconnection_channel * target);
    

  private:
    std::unique_ptr<_connection_manager> impl_;
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
