#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server : public iservice_factory
  {
  public:
    server();
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    async_task<> reset(
        const service_provider &sp,
        const std::string &root_user_name,
        const std::string &root_password);

    vds::async_task<> start_network(const vds::service_provider &sp);

    vds::async_task<> init_server(const vds::service_provider &sp, int port, const std::string &user_login,
                                      const std::string &user_password);

  private:
    class _server * const impl_;
  };
  
  class iserver
  {
  public:

  };
}

#endif // __VDS_SERVER_SERVER_H_
