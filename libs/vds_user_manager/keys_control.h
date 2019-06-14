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
      GET_EXPECTED_THROW(result, base64::to_bytes(root_id_));
      return result;
    }
    //common news read keys
    static const_data_buffer get_common_news_channel_id() {
      GET_EXPECTED_THROW(result, base64::to_bytes(common_news_channel_id_));
      return result;
    }

    static std::shared_ptr<asymmetric_public_key> get_common_news_read_public_key() {
      return load_public_key(common_news_read_public_key_);
    }

    static std::shared_ptr<asymmetric_private_key> get_common_news_read_private_key() {
      return load_private_key(common_news_read_private_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_common_news_write_public_key() {
      return load_public_key(common_news_write_public_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_common_news_admin_public_key() {
      return load_public_key(common_news_admin_public_key_);
    }

    //autoupdate keys
    static const_data_buffer get_autoupdate_channel_id() {
      GET_EXPECTED_THROW(result, base64::to_bytes(autoupdate_channel_id_));
      return result;
    }

    static std::shared_ptr<asymmetric_public_key> get_autoupdate_read_public_key() {
      return load_public_key(autoupdate_read_public_key_);
    }

    static std::shared_ptr<asymmetric_private_key> get_autoupdate_read_private_key() {
      return load_private_key(autoupdate_read_private_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_autoupdate_write_public_key() {
      return load_public_key(autoupdate_write_public_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_autoupdate_admin_public_key() {
      return load_public_key(autoupdate_admin_public_key_);
    }

    //web keys
    static const_data_buffer get_web_channel_id() {
      GET_EXPECTED_THROW(result, base64::to_bytes(web_channel_id_));
      return result;
    }

    static std::shared_ptr<asymmetric_public_key> get_web_read_public_key() {
      return load_public_key(web_read_public_key_);
    }

    static std::shared_ptr<asymmetric_private_key> get_web_read_private_key() {
      return load_private_key(web_read_private_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_web_write_public_key() {
      return load_public_key(web_write_public_key_);
    }

    static std::shared_ptr<asymmetric_public_key> get_web_admin_public_key() {
      return load_public_key(web_admin_public_key_);
    }

    static const std::string& auto_update_login();
    static const std::string& auto_update_password();

    static const std::string& web_login();
    static const std::string& web_password();

    class private_info_t {
    public:
      std::shared_ptr<asymmetric_private_key> common_news_write_private_key_;
      std::shared_ptr<asymmetric_private_key> common_news_admin_private_key_;
      std::shared_ptr<asymmetric_private_key> autoupdate_write_private_key_;
      std::shared_ptr<asymmetric_private_key> autoupdate_admin_private_key_;
      std::shared_ptr<asymmetric_private_key> web_write_private_key_;
      std::shared_ptr<asymmetric_private_key> web_admin_private_key_;
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
    static char common_news_channel_id_[65];
    static char common_news_read_public_key_[asymmetric_public_key::base64_size + 1];
    static char common_news_read_private_key_[asymmetric_private_key::base64_size + 1];
    static char common_news_write_public_key_[asymmetric_public_key::base64_size + 1];
    static char common_news_admin_public_key_[asymmetric_public_key::base64_size + 1];

    static char autoupdate_channel_id_[65];
    static char autoupdate_read_public_key_[asymmetric_public_key::base64_size + 1];
    static char autoupdate_read_private_key_[asymmetric_private_key::base64_size + 1];
    static char autoupdate_write_public_key_[asymmetric_public_key::base64_size + 1];
    static char autoupdate_admin_public_key_[asymmetric_public_key::base64_size + 1];

    static char web_channel_id_[65];
    static char web_read_public_key_[asymmetric_public_key::base64_size + 1];
    static char web_read_private_key_[asymmetric_private_key::base64_size + 1];
    static char web_write_public_key_[asymmetric_public_key::base64_size + 1];
    static char web_admin_public_key_[asymmetric_public_key::base64_size + 1];

    static std::shared_ptr<asymmetric_public_key> load_public_key(const char * data) {
      auto rb = base64::to_bytes(data);
      if (rb.has_error()) {
        throw std::runtime_error(rb.error()->what());
      }

      auto rc = asymmetric_public_key::parse_der(rb.value());
      if (rc.has_error()) {
        throw std::runtime_error(rc.error()->what());
      }

      return std::make_shared<asymmetric_public_key>(std::move(rc.value()));
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

#endif //__VDS_USER_MANAGER_KEYS_CONTROL_H_
