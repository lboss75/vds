#ifndef __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
#define __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {
  class _server_connection
    : public consensus_protocol::iserver_gateway
  {
  public:
    _server_connection(
      const service_provider & sp,
      server_connection * owner);
    ~_server_connection();

    void send(const std::list<std::string> & target_ids, const std::string & message) override;
    void broadcast(const std::string & message) override;

  private:
    service_provider sp_;
    server_connection * owner_;
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_PRIVATE_H_
