#ifndef __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
#define __VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"

namespace vds {
  namespace dht {
    namespace network {
      class client {
      public:

        void save_data(
            const service_provider & sp,
            const const_data_buffer & key,
            const const_data_buffer & value);

        void save_channel_state(
            const service_provider & sp,
            const const_data_buffer & channel_id,
            const const_data_buffer & leaf_id,
            const std::list<const_data_buffer> & basis);

        void query_data(
            const service_provider & sp,
            const const_data_buffer & key);

        void query_channel_state(
            const service_provider & sp,
            const const_data_buffer & channel_id);

      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DTH_NETWORK_CLIENT_H_
