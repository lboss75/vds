#include <db_model.h>
#include "stdafx.h"
#include "private/auth_session.h"

vds::auth_session::auth_session(const std::string &login, const std::string &password)
: login_(login),
  password_key_(symmetric_key::from_password(password)),
  password_hash_(hash::signature(hash::sha256(), password.c_str(), password.length())){

}

vds::async_task<> vds::auth_session::create_user(const vds::service_provider &sp) {
  return sp.get<db_model>()->async_transaction(sp, [sp, pthis = this->shared_from_this()](database_transaction & t) {
    pthis->user_mng_.create_root_user(
        sp,
        t,
        pthis->login_,
        pthis->password_key_,
        pthis->password_hash_);
    return true;
  });
}
