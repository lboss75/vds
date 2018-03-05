#ifndef __VDS_USER_MANAGER_DEVICE_ACTIVATION_H_
#define __VDS_USER_MANAGER_DEVICE_ACTIVATION_H_
#include "member_user.h"
#include "database.h"
#include "vds_debug.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class device_activation
  {
  public:
	  device_activation() = default;
	  device_activation(
      const std::string & user_name,
      const std::list<certificate> &certificate_chain,
      const asymmetric_private_key &private_key)
		  : user_name_(user_name),
      certificate_chain_(certificate_chain),
    private_key_(private_key){	
      vds_assert(!this->certificate_chain_.empty());
	  }

	  const asymmetric_private_key & private_key() const
	  {
		  return this->private_key_;
	  }

	  const std::list<certificate> & certificate_chain() const
	  {
		  return this->certificate_chain_;
	  }

    const std::string & user_name() const {
      return this->user_name_;
    }

	  const_data_buffer pack(const std::string & user_password) const;

	  static device_activation unpack(
				const const_data_buffer & data,
				const std::string & user_password);

  private:
    std::string user_name_;
	  std::list<certificate> certificate_chain_;
		asymmetric_private_key private_key_;

  };
}

#endif // __VDS_USER_MANAGER_DEVICE_ACTIVATION_H_
