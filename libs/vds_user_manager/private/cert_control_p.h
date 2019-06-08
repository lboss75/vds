#ifndef __VDS_USER_MANAGER_CERT_CONTROL_P_H_
#define __VDS_USER_MANAGER_CERT_CONTROL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _cert_control {
  public:
    static expected<certificate> create_cert(
      const vds::asymmetric_private_key & private_key);

    static expected<certificate> create_cert(
		const vds::asymmetric_private_key & private_key,
		const certificate & parent_user_cert,
		const asymmetric_private_key & parent_user_private_key);
  };
}

#endif //__VDS_USER_MANAGER_CERT_CONTROL_P_H_
