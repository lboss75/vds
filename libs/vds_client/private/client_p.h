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
      const certificate & /*server_certificate*/,
      const asymmetric_private_key & /*private_key*/>
      init_server(
        const service_provider & sp,
      const std::string & user_login,
      const std::string & user_password);
      
    async_task<> create_local_login(
      const service_provider & sp,
      const std::string & login,
      const std::string & password,
      const std::string & name);

    async_task<const std::string & /*version_id*/> upload_file(
      const service_provider & sp,
      const std::string & name,
      const filename & tmp_file);

    async_task<const vds::guid & /*version_id*/> download_data(
      const service_provider & sp,
      const std::string & name,
      const filename & target_file);

  private:
    client * owner_;
    foldername tmp_folder_;
    std::mutex tmp_folder_mutex_;
    size_t last_tmp_file_index_;

    async_task<
      const client_messages::certificate_and_key_response &/*response*/>
      authenticate(
        const service_provider & sp,
        const std::string & user_login,
        const std::string & user_password);
      
    async_task<const guid &/*version_id*/>
      looking_for_file(
        const service_provider & sp,
        const asymmetric_private_key & user_private_key,
        const guid & principal_id,
        const size_t order_num,
        const std::string & looking_file_name,
        const filename & target_file);
      
      async_task<> download_file(
        const service_provider & sp,
        const guid & version_id,
        const filename & tmp_file);

      static certificate load_user_certificate(const service_provider & sp);
      static asymmetric_private_key load_user_private_key(const service_provider & sp);
  };
}


#endif // __VDS_CLIENT_CLIENT_P_H_
