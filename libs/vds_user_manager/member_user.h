#ifndef __VDS_USER_MANAGER_MEMBER_USER_H_
#define __VDS_USER_MANAGER_MEMBER_USER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "user_channel.h"

namespace vds {
  class asymmetric_private_key;
  class iuser_manager_storage;
  class _member_user;
  
  class member_user
  {
  public:
    member_user(
      const std::shared_ptr<certificate> &user_cert,
      const std::shared_ptr<asymmetric_private_key> & private_key);

    const std::shared_ptr<certificate> & user_certificate() const;
    const std::shared_ptr<asymmetric_private_key> & private_key() const;

    //member_user create_user(
    //    const vds::asymmetric_private_key &owner_user_private_key,
    //    const std::string &user_name,
    //    const vds::asymmetric_private_key &private_key);

    user_channel create_channel(
      const service_provider *sp,
      transactions::transaction_block_builder &log,
      const std::string &name);

    _member_user * operator -> () const {
      return this->impl_;
    }

  private:
    _member_user * impl_;

    member_user(_member_user * impl);
  };
}

#endif // __VDS_USER_MANAGER_MEMBER_USER_H_
