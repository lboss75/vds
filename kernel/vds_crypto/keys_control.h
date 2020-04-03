#ifndef __VDS_USER_MANAGER_KEYS_CONTROL_H_
#define __VDS_USER_MANAGER_KEYS_CONTROL_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "asymmetriccrypto.h"
#include "encoding.h"

class mock_server;

namespace vds {

  class keys_control {
  public:
    static const_data_buffer root_id() {
      auto result = base64::to_bytes(root_id_);
      return result.value();
    }

    class private_info_t {
    public:
      std::shared_ptr<asymmetric_private_key> root_private_key_;

      expected<void> genereate_all();
    private:
      friend class mock_server;
    };

    static expected<void> genereate_all(
      const private_info_t & private_info);
    private:
      friend class get_root_app;
    friend class mock_server;

    static char root_id_[65];

    static std::shared_ptr<asymmetric_public_key> load_public_key(const char * data) {
      auto rb = base64::to_bytes(data);
#if __cpp_exceptions
      if (rb.has_error()) {
        throw std::runtime_error(rb.error()->what());
      }
#endif

      auto rc = asymmetric_public_key::parse_der(rb.value());
#if __cpp_exceptions
      if (rc.has_error()) {
        throw std::runtime_error(rc.error()->what());
      }
#endif

      return std::make_shared<asymmetric_public_key>(std::move(rc.value()));
    }
    static std::shared_ptr<asymmetric_private_key> load_private_key(const char * data) {
      auto rb = base64::to_bytes(data);
#if __cpp_exceptions
      if (rb.has_error()) {
        throw std::runtime_error(rb.error()->what());
      }
#endif

      auto rc = asymmetric_private_key::parse_der(rb.value(), std::string());
#if __cpp_exceptions
      if (rc.has_error()) {
        throw std::runtime_error(rc.error()->what());
      }
#endif

      return std::make_shared<asymmetric_private_key>(std::move(rc.value()));
    }
  };
}

#endif //__VDS_USER_MANAGER_KEYS_CONTROL_H_
