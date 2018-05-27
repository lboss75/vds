#ifndef __VDS_WEB_SERVER_AUTH_SESSION_H_
#define __VDS_WEB_SERVER_AUTH_SESSION_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/


#include "user_manager.h"
#include "service_provider.h"

namespace vds {
  class user_manager;

  class auth_session : public std::enable_shared_from_this<auth_session> {
  public:
    auth_session(
        const std::string & login,
        const std::string & password);

    async_task<> create_user(const service_provider & sp);

    async_task<> load(
      const service_provider & sp,
      const const_data_buffer & crypted_private_key);

    const std::shared_ptr<user_manager> & get_secured_context(
        const service_provider & sp) const {
      return this->user_mng_;
    }

  private:
    std::string login_;
    symmetric_key password_key_;
    const_data_buffer password_hash_;
    std::shared_ptr<user_manager> user_mng_;

    std::chrono::steady_clock::time_point last_update_;

  };
}

#endif //__VDS_WEB_SERVER_AUTH_SESSION_H_
