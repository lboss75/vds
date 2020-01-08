#ifndef __VDS_SERVER_CHANNEL_MANAGER_H_
#define __VDS_SERVER_CHANNEL_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "database.h"

namespace vds {
  class channel_manager
  {
  public:
    void register_channel(
      
      const database_transaction & t,
      const std::string & id,
      const std::string & name
    );
    
    void add_member(
      const std::string & id);
    
    void remove_member(
      const std::string & id);
  };
}

#endif // __VDS_SERVER_CHANNEL_MANAGER_H_
