#ifndef __VDS_DHT_NETWORK_DHT_SESSION_H_
#define __VDS_DHT_NETWORK_DHT_SESSION_H_


/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_task.h"
#include "const_data_buffer.h"
#include "dht_datagram_protocol.h"

namespace vds {
  namespace dht {
    namespace network {

      class dht_session : public dht_datagram_protocol<dht_session> {
      public:
        dht_session(
          const network_address & address,
          const const_data_buffer & this_node_id,
          const const_data_buffer & partner_node_id);

        async_task<> process_message(
            const service_provider & sp,
            uint8_t message_type,
            const const_data_buffer & message);

      };
    }
  }
}

#endif //__VDS_DHT_NETWORK_DHT_SESSION_H_
