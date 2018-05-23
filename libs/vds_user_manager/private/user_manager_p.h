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

    struct channel_info {
      std::map<std::string, certificate> read_certificates_;
      std::map<std::string, certificate> write_certificates_;

      std::map<std::string, asymmetric_private_key> read_private_keys_;
      std::map<std::string, asymmetric_private_key> write_private_keys_;

      std::string current_read_certificate_;
      std::string current_write_certificate_;

      user_channel::channel_type_t type_;
      std::string name_;
    };


    _user_manager(
        const const_data_buffer &dht_user_id,
        const symmetric_key &user_password_key,
        const const_data_buffer &user_password_hash);

    user_manager::login_state_t get_login_state() const {
      return this->login_state_;
    }


    const const_data_buffer &dht_user_id() const {
      return this->dht_user_id_;
    }

    const certificate &user_cert() const {
      return this->user_cert_;
    }

    const asymmetric_private_key &user_private_key() const {
      return this->user_private_key_;
    }

    void update(
        const service_provider &sp,
        class database_transaction &t);


    bool get_channel_write_certificate(
        const const_data_buffer &channel_id,
        std::string &name,
        certificate &read_certificate,
        asymmetric_private_key &read_key,
        certificate &write_certificate,
        asymmetric_private_key &write_key) const;

    std::string get_channel_name(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        return p->second.name_;
      }

      throw vds_exceptions::not_found();
    }

    user_channel::channel_type_t get_channel_type(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        return p->second.type_;
      }

      throw vds_exceptions::not_found();
    }

    certificate get_channel_write_cert(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p && !p->second.current_write_certificate_.empty()) {
        auto p1 = p->second.write_certificates_.find(p->second.current_write_certificate_);
        if (p->second.write_certificates_.end() != p1) {
          return p1->second;
        }
      }
      return certificate();
    }

    certificate get_channel_write_cert(
        const const_data_buffer &channel_id,
        const std::string &cert_subject) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        auto p1 = p->second.write_certificates_.find(cert_subject);
        if (p->second.write_certificates_.end() != p1) {
          return p1->second;
        }
      }

      return this->get_certificate(cert_subject);
    }

    asymmetric_private_key get_channel_write_key(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p && !p->second.current_write_certificate_.empty()) {
        auto p1 = p->second.write_private_keys_.find(p->second.current_write_certificate_);
        if (p->second.write_private_keys_.end() != p1) {
          return p1->second;
        }
      }
      return asymmetric_private_key();
    }

    asymmetric_private_key get_channel_write_key(const const_data_buffer &channel_id, const std::string &cert_subject) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        auto p1 = p->second.write_private_keys_.find(cert_subject);
        if (p->second.write_private_keys_.end() != p1) {
          return p1->second;
        }
      }
      return asymmetric_private_key();
    }

    certificate get_channel_read_cert(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p && !p->second.current_read_certificate_.empty()) {
        auto p1 = p->second.read_certificates_.find(p->second.current_read_certificate_);
        if (p->second.read_certificates_.end() != p1) {
          return p1->second;
        }
      }
      return certificate();
    }

    asymmetric_private_key get_channel_read_key(const const_data_buffer &channel_id) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p && !p->second.current_read_certificate_.empty()) {
        auto p1 = p->second.read_private_keys_.find(p->second.current_read_certificate_);
        if (p->second.read_private_keys_.end() != p1) {
          return p1->second;
        }
      }
      return asymmetric_private_key();
    }

    asymmetric_private_key get_channel_read_key(const const_data_buffer &channel_id, const std::string &cert_subject) const {
      auto p = this->channels_.find(channel_id);
      if (this->channels_.end() != p) {
        auto p1 = p->second.read_private_keys_.find(cert_subject);
        if (p->second.read_private_keys_.end() != p1) {
          return p1->second;
        }
      }
      return asymmetric_private_key();
    }

    void add_read_certificate(
        const const_data_buffer &channel_id,
        const certificate &read_cert,
        const asymmetric_private_key &read_private_key) {
      auto &cp = this->channels_[channel_id];

      auto id = read_cert.subject();
      cp.current_read_certificate_ = id;
      cp.read_certificates_[id] = read_cert;
      cp.read_private_keys_[id] = read_private_key;
    }

    certificate get_certificate(const std::string &id) const {
      auto p = this->certificate_chain_.find(id);
      if (this->certificate_chain_.end() == p) {
        return certificate();
      }

      return p->second;
    }

    const std::map<const_data_buffer, channel_info> &channels() const {
      return this->channels_;
    }

    void add_certificate(const certificate &cert);
    member_user get_current_user() const;

    const asymmetric_private_key & get_current_user_private_key() const {
      return this->user_private_key_;
    }

  private:
    const_data_buffer dht_user_id_;
    symmetric_key user_password_key_;
    const_data_buffer user_password_hash_;
    user_manager::login_state_t login_state_;

    std::set<std::string> processed_;
    certificate user_cert_;
    std::string user_name_;
    asymmetric_private_key user_private_key_;
    std::map<const_data_buffer, channel_info> channels_;
    std::map<std::string, certificate> certificate_chain_;
  };
}

#endif // __VDS_USER_MANAGER_USER_MANAGER_P_H_
