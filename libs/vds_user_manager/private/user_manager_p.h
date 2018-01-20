#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "user_manager.h"
#include "security_walker.h"

namespace vds {
  class _user_manager : public security_walker
  {
  public:
    _user_manager(
		const guid & common_channel_id,
		const guid & user_id,
		const certificate & user_cert,
		const asymmetric_private_key & user_private_key);

	member_user get_current_device(
		const service_provider &sp,
		asymmetric_private_key &device_private_key) const;

	user_channel get_channel(const guid &channel_id) const;

  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
