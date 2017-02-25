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
    server_connection();
    ~server_connection();

  private:
    const std::unique_ptr<class _server_connection> impl_;
  };
}

#endif // __VDS_SERVER_SERVER_CONNECTION_H_
