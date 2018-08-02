#ifndef __VDS_USER_MANAGER_CERT_CONTROL_H_
#define __VDS_USER_MANAGER_CERT_CONTROL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "asymmetriccrypto.h"
#include "encoding.h"

class mock_server;

namespace vds {

  class cert_control {
  public:
    static certificate get_root_certificate() {
      return certificate::parse_der(base64::to_bytes(root_certificate_));
    }

    static asymmetric_private_key get_root_private_key(const std::string & root_password) {
      return asymmetric_private_key::parse_der(base64::to_bytes(root_private_key_), root_password);
    }

  private:
    friend class get_root_app;
    friend class mock_server;

    static char root_certificate_[1821];
    static char root_private_key_[3137];


    static void genereate_all(const std::string& root_login, const std::string& root_password);
  };
}

#endif //__VDS_USER_MANAGER_CERT_CONTROL_H_
