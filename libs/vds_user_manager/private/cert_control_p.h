#ifndef __VDS_USER_MANAGER_CERT_CONTROL_P_H_
#define __VDS_USER_MANAGER_CERT_CONTROL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {

  class _cert_control {
  public:
    static certificate create_root(
        const guid & id,
        const std::string & name,
        const vds::asymmetric_private_key & private_key);

    static certificate create(
        const guid & id,
        const std::string & name,
        const vds::asymmetric_private_key & private_key,
        const guid & user_id,
        const certificate & user_cert,
        const asymmetric_private_key & user_private_key);

  private:

  };
}

#endif //__VDS_USER_MANAGER_CERT_CONTROL_P_H_
