#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server;

  class server : public iservice_factory
  {
  public:
    server();
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    void set_port(int port);

    async_task<> reset(
        const service_provider &sp,
        const std::string &root_user_name,
        const std::string &root_password);

  private:
    _server * const impl_;
  };
  
  class iserver
  {
  public:

  };
}

#endif // __VDS_SERVER_SERVER_H_
