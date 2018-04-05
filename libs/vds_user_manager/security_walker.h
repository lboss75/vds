#ifndef __VDS_USER_MANAGER_SECURITY_WALKER_H_
#define __VDS_USER_MANAGER_SECURITY_WALKER_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <map>
#include "guid.h"
#include "asymmetriccrypto.h"
#include "vds_exceptions.h"
#include "cert_control.h"
#include "user_channel.h"

namespace vds {
  class security_walker {
  public:
    enum class login_state_t {
      waiting_channel,
      login_sucessful,
      login_failed
    };

    struct channel_info {
      std::map<guid, certificate> read_certificates_;
      std::map<guid, certificate> write_certificates_;

      std::map<guid, asymmetric_private_key> read_private_keys_;
      std::map<guid, asymmetric_private_key> write_private_keys_;

      guid current_read_certificate_;
      guid current_write_certificate_;

      user_channel::channel_type_t type_;
      std::string name_;
    };


    security_walker(
        const const_data_buffer & dht_user_id,
        const symmetric_key & user_password_key,
        const const_data_buffer & user_password_hash);

    login_state_t get_login_state() const {
      return this->login_state_;
    }


    const guid & user_id() const {
      return this->user_id_;
    }

    const const_data_buffer & dht_user_id() const {
      return this->dht_user_id_;
    }
    
    const certificate & user_cert() const {
      return this->user_cert_;
    }

    const asymmetric_private_key & user_private_key() const {
      return this->user_private_key_;
    }

    void update(
		const service_provider & sp, 
		class database_transaction &t);


    bool get_channel_write_certificate(
		const guid &channel_id,
        std::string & name,
		certificate & read_certificate,
		asymmetric_private_key &read_key,
        certificate &write_certificate,
        asymmetric_private_key &write_key) const;

	std::string get_channel_name(const const_data_buffer &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p) {
			return  p->second.name_;
		}

		throw vds_exceptions::not_found();
	}

  user_channel::channel_type_t get_channel_type(const const_data_buffer &channel_id) const {
    auto p = this->channels_.find(channel_id);
    if (this->channels_.end() != p) {
      return  p->second.type_;
    }

    throw vds_exceptions::not_found();
  }

	certificate get_channel_write_cert(const const_data_buffer &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_write_certificate_) {
			auto p1 = p->second.write_certificates_.find(p->second.current_write_certificate_);
			if (p->second.write_certificates_.end() != p1) {
				return p1->second;
			}
		}
		return certificate();
	}
		certificate get_channel_write_cert(
        const const_data_buffer &channel_id,
        const guid &cert_id) const {
			auto p = this->channels_.find(channel_id);
			if (this->channels_.end() != p) {
				auto p1 = p->second.write_certificates_.find(cert_id);
				if (p->second.write_certificates_.end() != p1) {
					return p1->second;
				}
			}

			return this->get_certificate(cert_id);
		}

	asymmetric_private_key get_channel_write_key(const const_data_buffer &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_write_certificate_) {
			auto p1 = p->second.write_private_keys_.find(p->second.current_write_certificate_);
			if (p->second.write_private_keys_.end() != p1) {
				return p1->second;
			}
		}
		return asymmetric_private_key();
	}

	asymmetric_private_key get_channel_write_key(const const_data_buffer &channel_id, const guid & cert_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p) {
			auto p1 = p->second.write_private_keys_.find(cert_id);
			if (p->second.write_private_keys_.end() != p1) {
				return p1->second;
			}
		}
		return asymmetric_private_key();
	}

	certificate get_channel_read_cert(const const_data_buffer &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_read_certificate_) {
			auto p1 = p->second.read_certificates_.find(p->second.current_read_certificate_);
			if (p->second.read_certificates_.end() != p1) {
				return p1->second;
			}
		}
		return certificate();
	}

	asymmetric_private_key get_channel_read_key(const const_data_buffer &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_read_certificate_) {
			auto p1 = p->second.read_private_keys_.find(p->second.current_read_certificate_);
			if (p->second.read_private_keys_.end() != p1) {
				return p1->second;
			}
		}
		return asymmetric_private_key();
	}

		asymmetric_private_key get_channel_read_key(const const_data_buffer &channel_id, const guid &cert_id) const {
			auto p = this->channels_.find(channel_id);
			if (this->channels_.end() != p) {
				auto p1 = p->second.read_private_keys_.find(cert_id);
				if (p->second.read_private_keys_.end() != p1) {
					return p1->second;
				}
			}
			return asymmetric_private_key();
		}

	void add_read_certificate(
		const const_data_buffer & channel_id,
		const certificate & read_cert,
		const asymmetric_private_key & read_private_key) {
		auto & cp = this->channels_[channel_id];

		auto id = cert_control::get_id(read_cert);
		cp.current_read_certificate_ = id;
		cp.read_certificates_[id] = read_cert;
		cp.read_private_keys_[id] = read_private_key;
	}

	certificate get_certificate(const guid & id)  const {
		auto p = this->certificate_chain_.find(id);
		if (this->certificate_chain_.end() == p) {
			return certificate();
		}

		return p->second;
	}

  const std::map<const_data_buffer, channel_info> & channels() const {
    return this->channels_;
	}

  void add_certificate(const certificate &cert);

	private:
		const_data_buffer dht_user_id_;
		symmetric_key user_password_key_;
    const_data_buffer user_password_hash_;
    login_state_t login_state_;

    std::set<std::string> processed_;
    guid user_id_;
    certificate user_cert_;
    std::string user_name_;
    asymmetric_private_key user_private_key_;
    std::map<const_data_buffer, channel_info> channels_;
		std::map<guid, certificate> certificate_chain_;
	};
}

#endif //__VDS_USER_MANAGER_SECURITY_WALKER_H_
