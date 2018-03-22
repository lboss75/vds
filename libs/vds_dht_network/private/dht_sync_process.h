#ifndef __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#define __VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
#include "service_provider.h"
#include "database.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  namespace dht {
    namespace network {
      class sync_process {
      public:
        void do_sync(
          const service_provider & sp,
          database_transaction & t);

      private:
        void sync_local_channels(
          const service_provider & sp,
          database_transaction & t);
      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_SYNC_PROCESS_H_
