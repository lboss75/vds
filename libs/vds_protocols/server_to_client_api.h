#ifndef __VDS_PROTOCOLS_SERVER_TO_CLIENT_API_H_
#define __VDS_PROTOCOLS_SERVER_TO_CLIENT_API_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class server_to_client_api
  {
  public:
    async_task<void(const std::string & certificate_body, const data_buffer & private_key_body)> get_certificate_and_key(const std::string & object_name, const data_buffer & password_hash);
  };

}

#endif // __VDS_PROTOCOLS_SERVER_TO_CLIENT_API_H_
