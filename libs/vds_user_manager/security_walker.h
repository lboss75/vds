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

namespace vds {
  class security_walker {
  public:
    security_walker(
        const guid & user_id,
        const certificate & user_cert,
        const asymmetric_private_key & user_private_key);

    const guid & user_id() const {
      return this->user_id_;
    }

    const certificate & user_cert() const {
      return this->user_cert_;
    }

    const asymmetric_private_key & user_private_key() const {
      return this->user_private_key_;
    }

    void load(
		const service_provider & sp, 
		class database_transaction &t);

    const_data_buffer decrypt_message(
        const service_provider & parent_scope,
        const guid & channel_id,
        int message_id,
        const guid & read_cert_id,
        const guid & write_cert_id,
        const const_data_buffer & message_data,
        const const_data_buffer & signature);


      void apply(
		const service_provider & sp,
        const guid & channel_id,
        int message_id,
        const guid & read_cert_id,
        const guid & write_cert_id,
        const const_data_buffer & message,
        const const_data_buffer & signature);

    bool get_channel_write_certificate(
		const guid &channel_id,
        std::string & name,
		certificate & read_certificate,
		asymmetric_private_key &read_key,
        certificate &write_certificate,
        asymmetric_private_key &write_key) const;

	std::string get_channel_name(const guid &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p) {
			return  p->second.name_;
		}

		throw vds_exceptions::not_found();
	}

	certificate get_channel_write_cert(const guid &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_write_certificate_) {
			auto p1 = p->second.write_certificates_.find(p->second.current_write_certificate_);
			if (p->second.write_certificates_.end() != p1) {
				return p1->second;
			}
		}
		return certificate();
	}
		certificate get_channel_write_cert(const guid &channel_id, const guid &cert_id) const {
			auto p = this->channels_.find(channel_id);
			if (this->channels_.end() != p) {
				auto p1 = p->second.write_certificates_.find(cert_id);
				if (p->second.write_certificates_.end() != p1) {
					return p1->second;
				}
			}

			return certificate();
		}

	asymmetric_private_key get_channel_write_key(const guid &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_write_certificate_) {
			auto p1 = p->second.write_private_keys_.find(p->second.current_write_certificate_);
			if (p->second.write_private_keys_.end() != p1) {
				return p1->second;
			}
		}
		return asymmetric_private_key();
	}

	certificate get_channel_read_cert(const guid &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_read_certificate_) {
			auto p1 = p->second.read_certificates_.find(p->second.current_read_certificate_);
			if (p->second.read_certificates_.end() != p1) {
				return p1->second;
			}
		}
		return certificate();
	}

	asymmetric_private_key get_channel_read_key(const guid &channel_id) const {
		auto p = this->channels_.find(channel_id);
		if (this->channels_.end() != p && p->second.current_read_certificate_) {
			auto p1 = p->second.read_private_keys_.find(p->second.current_read_certificate_);
			if (p->second.read_private_keys_.end() != p1) {
				return p1->second;
			}
		}
		return asymmetric_private_key();
	}

	asymmetric_private_key get_common_channel_read_key(const guid & cert_id) const {
		auto p = this->channels_.find(this->common_channel_id_);
		if (this->channels_.end() == p) {
			throw std::runtime_error("Invalid logic");
		}

		auto p1 = p->second.read_private_keys_.find(cert_id);
		if (p->second.read_private_keys_.end() == p1) {
			throw std::runtime_error("Invalid logic");
		}

		return p1->second;
	}

	guid get_common_channel_id() const {
		return this->common_channel_id_;
	}

	void add_read_certificate(
		const guid & channel_id,
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


  private:
    const guid user_id_;
    const certificate user_cert_;
    const asymmetric_private_key user_private_key_;

    struct channel_info {
      std::map<guid, certificate> read_certificates_;
      std::map<guid, certificate> write_certificates_;

      std::map<guid, asymmetric_private_key> read_private_keys_;
      std::map<guid, asymmetric_private_key> write_private_keys_;

      guid current_read_certificate_;
	    guid current_write_certificate_;

      std::string name_;
    };

    std::map<guid, channel_info> channels_;

	std::map<guid, certificate> certificate_chain_;

  };
}

#endif //__VDS_USER_MANAGER_SECURITY_WALKER_H_
