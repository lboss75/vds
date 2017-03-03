#ifndef __VDS_SERVER_SERVER_CONNECTION_H_
#define __VDS_SERVER_SERVER_CONNECTION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_connection
  {
  public:
    server_connection(const service_provider & sp);
    ~server_connection();

    consensus_protocol::iserver_gateway & get_server_gateway() const;

  private:
    const std::unique_ptr<class _server_connection> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_H_
