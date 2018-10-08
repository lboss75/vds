#include <db_model.h>
#include "stdafx.h"
#include "private/auth_session.h"
#include "dht_object_id.h"
#include "dht_network_client.h"

vds::auth_session::auth_session(
  const service_provider * sp,
  const std::string &login, const std::string &password)
: login_(login),
  password_(password),
  user_mng_(new user_manager(sp)) {

}


std::future<void> vds::auth_session::load(const service_provider * sp) {

  return sp->get<db_model>()->async_transaction([sp, pthis = this->shared_from_this()](database_transaction & t) {
    pthis->user_mng_->load(
      t,
      pthis->login_,
      pthis->password_);
  });
}

vds::user_manager::login_state_t vds::auth_session::get_login_state() const {
  return this->user_mng_->get_login_state();
}

const std::string& vds::auth_session::user_name() {
  return this->user_mng_->user_name();
}
