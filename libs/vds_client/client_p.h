#ifndef __VDS_CLIENT_CLIENT_P_H_
#define __VDS_CLIENT_CLIENT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client.h"

namespace vds {
  class _client : public iclient
  {
  public:
    _client(client * owner);
    
    async_task<
      const vds::certificate & /*server_certificate*/,
      const vds::asymmetric_private_key & /*private_key*/>
      init_server(
        const service_provider & sp,
      const std::string & user_login,
      const std::string & user_password);

    async_task<const std::string& /*version_id*/> upload_file(
      const service_provider & sp,
      const std::string & login,
      const std::string & password,
      const std::string & name,
      const filename & tmp_file);

    async_task<filename> download_data(
      const service_provider & sp,
      const std::string & login,
      const std::string & password,
      const std::string & name);

  private:
    client * owner_;

    async_task<
      const vds::certificate & /*user_certificate*/,
      const vds::asymmetric_private_key & /*user_private_key*/>
      authenticate(
        const service_provider & sp,
        const std::string & user_login,
        const std::string & user_password);
  };
}


#endif // __VDS_CLIENT_CLIENT_P_H_
