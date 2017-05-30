#ifndef __VDS_CLIENT_CLIENT_P_H_
#define __VDS_CLIENT_CLIENT_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "client.h"
#include "foldername.h"

namespace vds {
  class _client : public iclient
  {
  public:
    _client(client * owner);

    void start(const service_provider & sp);
    void stop(const service_provider & sp);

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

    async_task<> download_data(
      const service_provider & sp,
      const std::string & login,
      const std::string & password,
      const std::string & name,
      const filename & target_file);

  private:
    client * owner_;
    foldername tmp_folder_;
    std::mutex tmp_folder_mutex_;
    size_t last_tmp_file_index_;

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
