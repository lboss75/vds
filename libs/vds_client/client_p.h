#ifndef __VDS_CLIENT_CLIENT_P_H_
#define __VDS_CLIENT_CLIENT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client.h"

namespace vds {
  class _client
  {
  public:
    _client(const service_provider & sp, client * owner);
    
    async_task<
      const vds::certificate & /*server_certificate*/,
      const vds::asymmetric_private_key & /*private_key*/>
      init_server(
      const std::string & user_login,
      const std::string & user_password);

    async_task<const std::string& /*version_id*/> upload_file(
      const std::string & login,
      const std::string & password,
      const std::string & name,
      const void * data,
      size_t data_size);

    async_task<data_buffer &&> download_data(
      const std::string & login,
      const std::string & password,
      const std::string & name);

  private:
    client * owner_;
    service_provider sp_;
    logger log_;

    async_task<
      const vds::certificate & /*user_certificate*/,
      const vds::asymmetric_private_key & /*user_private_key*/>

      authenticate(
        const std::string & user_login,
        const std::string & user_password);
  };
}


#endif // __VDS_CLIENT_CLIENT_P_H_
