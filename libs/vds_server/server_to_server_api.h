#ifndef __VDS_PROTOCOLS_SERVER_TO_SERVER_API_H_
#define __VDS_PROTOCOLS_SERVER_TO_SERVER_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class _connection_manager;
  class connection_session;

  class server_to_server_api
  {
  public:
    void process_message(
      const service_provider & sp,
      database_transaction & t,
      _connection_manager * con_man,
      const connection_session & session,
      uint32_t message_type_id,
      const const_data_buffer & binary_form);
  };
}

#endif // __VDS_PROTOCOLS_SERVER_TO_SERVER_API_H_
