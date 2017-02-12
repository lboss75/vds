#ifndef __VDS_SERVER_SERVER_H_
#define __VDS_SERVER_SERVER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "server_logic.h"

namespace vds {
  class server : public iservice
  {
  public:
    void register_services(service_registrator &) override;
    void start(const service_provider &) override;
    void stop(const service_provider &) override;
    
  private:
    friend class iserver;
  };
  
  class iserver
  {
  public:
    
  private:
    server * owner_;
  };
}

#endif // __VDS_SERVER_SERVER_H_
