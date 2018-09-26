#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "cert_control.h"
#include "vds_exceptions.h"
#include "user_manager.h"

namespace vds {
  class _user_manager : public std::enable_shared_from_this<_user_manager> {
  public:
    _user_manager(
        const std::string &user_credentials_key,
        const asymmetric_private_key & user_private_key);

    const std::string & user_credentials_key() const {
      return this->user_credentials_key_;
    }

    const certificate &user_cert() const {
      return this->user_cert_;
    }

    const asymmetric_private_key &user_private_key() const {
      return this->user_private_key_;
    }

    user_manager::login_state_t get_login_state() const {
      return this->login_state_;
    }

    void update(
        const service_provider &sp,
        class database_transaction &t);


    user_channel get_channel(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        return p->second;
      }

      return user_channel();
    }

    const std::map<const_data_buffer, user_channel> &channels() const {
      return this->channels_;
    }

    certificate get_certificate(const std::string &id) const {
      auto p = this->certificate_chain_.find(id);
      if (this->certificate_chain_.end() == p) {
        return certificate();
      }

      return p->second;
    }

    void add_certificate(const certificate &cert);
    member_user get_current_user() const;

    const asymmetric_private_key & get_current_user_private_key() const {
      return this->user_private_key_;
    }

    std::future<bool> approve_join_request(
      const service_provider& sp,
      const const_data_buffer& data);

    const std::string& user_name() const;

    static bool parse_join_request(
        const service_provider& sp,
        const const_data_buffer & data,
        std::string & userName,
        std::string & userEmail);

    std::future<user_channel> create_channel(
      const service_provider& sp,
      const std::string& name) ;

  private:
    std::string user_credentials_key_;
    asymmetric_private_key user_private_key_;
    user_manager::login_state_t login_state_;

    certificate root_user_cert_;
    std::string root_user_name_;

    std::set<const_data_buffer> processed_;
    certificate user_cert_;
    std::string user_name_;
    std::map<const_data_buffer, user_channel> channels_;
    std::map<std::string, certificate> certificate_chain_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
