#ifndef __VDS_USER_MANAGER_USER_INVITATION_H_
#define __VDS_USER_MANAGER_USER_INVITATION_H_
#include "member_user.h"
#include "database.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class user_invitation
  {
  public:
	  user_invitation() = default;
	  user_invitation(const std::list<certificate> &certificate_chain, const asymmetric_private_key &private_key)
		  : private_key_(private_key),
		  certificate_chain_(certificate_chain) {		  
	  }

	  const asymmetric_private_key & private_key() const
	  {
		  return this->private_key_;
	  }

	  const std::list<certificate> & certificate_chain() const
	  {
		  return this->certificate_chain_;
	  }

	  const_data_buffer pack(const std::string & user_password) const;

	  static user_invitation unpack(
				const const_data_buffer & data,
				const std::string & user_password);

  private:
	  std::list<certificate> certificate_chain_;
		asymmetric_private_key private_key_;

  };
}

#endif // __VDS_USER_MANAGER_USER_INVITATION_H_
