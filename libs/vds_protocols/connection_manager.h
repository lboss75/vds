#ifndef __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
#define __VDS_PROTOCOLS_CONNECTION_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _connection_manager;

  class connection_manager : public iservice
  {
  public:
    connection_manager();
    ~connection_manager();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

  private:
    std::unique_ptr<_connection_manager> impl_;
  };
  
  class iconnection_manager
  {
  public:
    iconnection_manager(_connection_manager * owner);
    
    async_task<> start_server(const std::string & address);
    
  private:
    _connection_manager * const owner_;
  };
}
#endif // __VDS_PROTOCOLS_CONNECTION_MANAGER_H_
