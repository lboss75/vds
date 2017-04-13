#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server;

  class server : public iservice
  {
  public:
    server(bool for_init = false);
    ~server();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

    void set_port(size_t port);
    
  private:
    bool for_init_;
    _server * const impl_;
  };
  
  class iserver
  {
  public:
    iserver(server * owner);

  private:
    server * owner_;
  };
}

#endif // __VDS_SERVER_SERVER_H_
