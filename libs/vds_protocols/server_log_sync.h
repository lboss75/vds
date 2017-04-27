#ifndef __VDS_PROTOCOLS_SERVER_LOG_SYNC_H_
#define __VDS_PROTOCOLS_SERVER_LOG_SYNC_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _server_log_sync;
  class server_log_sync : public iservice_factory
  {
  public:
    server_log_sync();
    ~server_log_sync();
    
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;

  private:
    std::unique_ptr<_server_log_sync> impl_;
  };  
}

#endif // __VDS_PROTOCOLS_SERVER_LOG_SYNC_H_
