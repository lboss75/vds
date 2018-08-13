#ifndef __VDS_DHT_NETWORK_IMESSAGE_MAP_H_
#define __VDS_DHT_NETWORK_IMESSAGE_MAP_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "service_provider.h"
#include "const_data_buffer.h"
#include "dht_network_client.h"

namespace vds {
  namespace dht {
    namespace network {
      class dht_session;

      class imessage_map {
      public:
				virtual async_task<> process_message(
						const service_provider& scope,
            const std::shared_ptr<dht_session> & session,
						uint8_t message_type,
						const const_data_buffer& message_data) = 0;
				};
    }
  }
}

#endif //__VDS_DHT_NETWORK_IMESSAGE_MAP_H_
