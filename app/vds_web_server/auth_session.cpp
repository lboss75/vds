#include <db_model.h>
#include "stdafx.h"
#include "private/auth_session.h"
#include "dht_object_id.h"
#include "dht_network_client.h"

vds::auth_session::auth_session(const std::string &login, const std::string &password)
: login_(login),
  user_credentials_key_(dht::dht_object_id::user_credentials_to_key(login, password)),
  password_key_(symmetric_key::from_password(password)),
  password_hash_(hash::signature(hash::sha256(), password.c_str(), password.length())),
  user_mng_(new user_manager()) {

}


vds::async_task<> vds::auth_session::load(const service_provider& sp, const const_data_buffer & crypted_private_key) {

  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this(), crypted_private_key](database_transaction & t) {
      pthis->user_mng_->load(
        sp,
        t,
        pthis->user_credentials_key_,
        asymmetric_private_key::parse_der(symmetric_decrypt::decrypt(pthis->password_key_, crypted_private_key), std::string()));
  });
}

vds::user_manager::login_state_t vds::auth_session::get_login_state() const {
  return this->user_mng_->get_login_state();
}
