#ifndef __VDS_SERVER_CHANNEL_MANAGER_H_
#define __VDS_SERVER_CHANNEL_MANAGER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "service_provider.h"
#include "database.h"
#include "guid.h"

namespace vds {
  class channel_manager
  {
  public:
    void register_channel(
      const service_provider & sp,
      const database_transaction & t,
      const guid & id,
      const std::string & name
    );
    
    void add_member(
      const guid & id);
    
    void remove_member(
      const guid & id);
    
  private:
  };
}

#endif // __VDS_SERVER_CHANNEL_MANAGER_H_
