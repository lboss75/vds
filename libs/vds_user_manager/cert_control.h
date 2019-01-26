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
    static std::shared_ptr<certificate> get_root_certificate() {
      return load_certificate(root_certificate_);
    }

    //common news read certificate
    static std::shared_ptr<certificate> get_common_news_read_certificate() {
      return load_certificate(common_news_read_certificate_);
    }

    static std::shared_ptr<asymmetric_private_key> get_common_news_read_private_key() {
      return load_private_key(common_news_read_private_key_);
    }

    static std::shared_ptr<certificate> get_common_news_write_certificate() {
      return load_certificate(common_news_write_certificate_);
    }

    static std::shared_ptr<certificate> get_common_news_admin_certificate() {
      return load_certificate(common_news_admin_certificate_);
    }
    //common storage
    static std::shared_ptr<certificate> get_storage_certificate() {
      return load_certificate(common_storage_certificate_);
    }

    static std::shared_ptr<asymmetric_private_key> get_common_storage_private_key() {
      return load_private_key(common_storage_private_key_);
    }

    class private_info_t {
    public:
      std::shared_ptr<asymmetric_private_key> root_private_key_;
      std::shared_ptr<asymmetric_private_key> common_news_write_private_key_;
      std::shared_ptr<asymmetric_private_key> common_news_admin_private_key_;

      expected<void> genereate_all();
    private:
      friend class get_root_app;
      friend class mock_server;
    };

    static expected<void> genereate_all(
        const std::string& root_login,
        const std::string& root_password,
        const private_info_t & private_info);
    private:
    friend class get_root_app;
    friend class mock_server;

    static char root_certificate_[1821];

    static char common_news_read_certificate_[1821];
    static char common_news_read_private_key_[3137];
    static char common_news_write_certificate_[1821];
    static char common_news_admin_certificate_[1821];

    static char common_storage_certificate_[1821];
    static char common_storage_private_key_[3137];

    static std::shared_ptr<certificate> load_certificate(const char * data) {
      auto rb = base64::to_bytes(data);
      if (rb.has_error()) {
        throw std::runtime_error(rb.error()->what());
      }

      auto rc = certificate::parse_der(rb.value());
      if (rc.has_error()) {
        throw std::runtime_error(rc.error()->what());
      }

      return std::make_shared<certificate>(std::move(rc.value()));
    }
    static std::shared_ptr<asymmetric_private_key> load_private_key(const char * data) {
      auto rb = base64::to_bytes(data);
      if (rb.has_error()) {
        throw std::runtime_error(rb.error()->what());
      }

      auto rc = asymmetric_private_key::parse_der(rb.value(), std::string());
      if (rc.has_error()) {
        throw std::runtime_error(rc.error()->what());
      }

      return std::make_shared<asymmetric_private_key>(std::move(rc.value()));
    }
  };
}

#endif //__VDS_USER_MANAGER_CERT_CONTROL_H_
