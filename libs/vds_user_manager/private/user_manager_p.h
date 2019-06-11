#ifndef __VDS_USER_MANAGER_USER_MANAGER_P_H_
#define __VDS_USER_MANAGER_USER_MANAGER_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "keys_control.h"
#include "vds_exceptions.h"
#include "user_manager.h"
#include "user_wallet.h"

namespace vds {
  class _user_manager : public std::enable_shared_from_this<_user_manager> {
  public:
    expected<void> create(
      const service_provider * sp,
      const std::string & user_login,
      const std::string & user_password);

    const std::string & user_credentials_key() const {
      return this->user_credentials_key_;
    }

    const std::shared_ptr<asymmetric_public_key> &user_cert() const {
      return this->user_cert_;
    }

    const std::shared_ptr<asymmetric_private_key> &user_private_key() const {
      return this->user_private_key_;
    }

    user_manager::login_state_t get_login_state() const {
      return this->login_state_;
    }

    expected<void> update(        
        class database_read_transaction &t);


    std::shared_ptr<user_channel> get_channel(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        return p->second;
      }

      return std::shared_ptr<user_channel>();
    }

    const std::map<const_data_buffer, std::shared_ptr<user_channel>> &channels() const {
      return this->channels_;
    }

    std::shared_ptr<asymmetric_public_key> get_certificate(const const_data_buffer &id) const {
      auto p = this->certificate_chain_.find(id);
      if (this->certificate_chain_.end() == p) {
        return std::shared_ptr<asymmetric_public_key>();
      }

      return p->second;
    }

    expected<void> add_public_key(const std::shared_ptr<asymmetric_public_key> &public_key);
    member_user get_current_user() const;

    const std::shared_ptr<asymmetric_private_key> & get_current_user_private_key() const {
      return this->user_private_key_;
    }

    const std::string& user_name() const;

    async_task<expected<user_channel>> create_channel(
      const std::string & channel_type,
      const std::string& name) ;

    const std::list<std::shared_ptr<vds::user_wallet>>& wallets() const {
      return this->wallets_;
    }

  private:
    const service_provider * sp_;
    std::string user_credentials_key_;
    std::shared_ptr<asymmetric_private_key> user_private_key_;
    user_manager::login_state_t login_state_;

    std::set<const_data_buffer> processed_;
    std::shared_ptr<asymmetric_public_key> user_cert_;
    std::string user_name_;
    std::string user_password_;
    std::map<const_data_buffer, std::shared_ptr<user_channel>> channels_;
    std::map<const_data_buffer, std::shared_ptr<asymmetric_public_key>> certificate_chain_;
    std::list<std::shared_ptr<user_wallet>> wallets_;

    expected<bool> process_create_user_transaction(
      const transactions::create_user_transaction & message);

    expected<bool> process_channel_message(
      const transactions::channel_message & message,
      std::set<const_data_buffer> & new_channels,
      std::chrono::system_clock::time_point tp);
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
