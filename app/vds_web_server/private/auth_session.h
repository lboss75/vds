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
        const service_provider * sp,
        const std::string & session_id,
        const std::string & login,
        const std::string & password);

    vds::async_task<void> load(const service_provider * sp);
    vds::async_task<void> update();

    const std::shared_ptr<user_manager> & get_secured_context(
        ) const {
      return this->user_mng_;
    }

    const std::string & id() const {
      return this->session_id_;
    }
    user_manager::login_state_t get_login_state() const;
    const std::string & user_name();

  private:
    std::string session_id_;
    std::string login_;
    std::string password_;
    std::shared_ptr<user_manager> user_mng_;

    std::chrono::steady_clock::time_point last_update_;

  };
}

#endif //__VDS_WEB_SERVER_AUTH_SESSION_H_
