#ifndef __VDS_USER_MANAGER_USER_CHANNEL_P_H_
#define __VDS_USER_MANAGER_USER_CHANNEL_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <memory>
#include "member_user.h"

namespace vds {

  class _user_channel
  {
  public:
      _user_channel();
      _user_channel(
        const const_data_buffer &id,
        user_channel::channel_type_t channel_type,
		    const std::string & name,
        const std::shared_ptr<certificate> & read_cert,
        const std::shared_ptr<asymmetric_private_key> & read_private_key,
        const std::shared_ptr<certificate> & write_cert,
        const std::shared_ptr<asymmetric_private_key> & write_private_key);

    const const_data_buffer &id() const { return this->id_;}
    user_channel::channel_type_t channel_type() const { return this->channel_type_; }
    const std::string &name() const { return this->name_; }

    const std::shared_ptr<vds::certificate> & read_cert() const {
      if (this->current_read_certificate_.empty()) {
        throw std::invalid_argument("vds::_user_channel::add_reader");
      }
      return this->read_certificates_.find(this->current_read_certificate_)->second;
    }

    std::shared_ptr<certificate> read_cert(const std::string & subject) const {
      auto p = this->read_certificates_.find(subject);
      if (this->read_certificates_.end() == p) {
        return std::shared_ptr<certificate>();
      }

      return p->second;
    }

    const std::shared_ptr<certificate> & write_cert() const {
      if (this->current_read_certificate_.empty() || this->current_write_certificate_.empty()) {
        throw std::invalid_argument("vds::_user_channel::add_reader");
      }
      return this->write_certificates_.find(this->current_write_certificate_)->second;
    }

    const std::shared_ptr<asymmetric_private_key> & read_private_key() const {
      if (this->current_read_certificate_.empty()) {
        throw std::invalid_argument("vds::_user_channel::read_private_key");
      }
      return this->read_private_keys_.find(this->current_read_certificate_)->second;
    }

    const std::shared_ptr<asymmetric_private_key> & write_private_key() const {
      if (this->current_write_certificate_.empty()) {
        throw std::invalid_argument("vds::_user_channel::write_private_key");
      }
      return this->write_private_keys_.find(this->current_write_certificate_)->second;
    }

	  void add_reader(
	    transactions::transaction_block_builder& playback,
	    const member_user& member_user,
	    const vds::member_user& owner_user,
	    const asymmetric_private_key& owner_private_key) const;

	  void add_writer(
	    transactions::transaction_block_builder& playback,
	    const member_user& member_user,
	    const vds::member_user& owner_user) const;

    void add_writer(
      transactions::transaction_block_builder& playback,
      const std::string & name,
      const member_user& member_user,
      const vds::member_user& owner_user) const;

    std::shared_ptr<asymmetric_private_key> read_cert_private_key(const std::string& cert_subject) {
      auto p = this->read_private_keys_.find(cert_subject);
      if (this->read_private_keys_.end() == p) {
        return std::shared_ptr<asymmetric_private_key>();
      }

      return p->second;
    }

    static std::shared_ptr <user_channel> import_personal_channel(
      const service_provider & sp,
      const std::shared_ptr<certificate> & user_cert,
      const std::shared_ptr<asymmetric_private_key> & user_private_key);

    template<typename item_type>
    void add_log(
      transactions::transaction_block_builder & log,
      const member_user & writter,
      item_type && item) {

      auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

      binary_serializer s;
      s
        << (uint8_t)item_type::message_id;
      item.visit(_serialize_visitor(s));

      log.add(
        transactions::channel_message(
          this->id_,
          this->read_cert()->subject(),
          writter.user_certificate()->subject(),
          this->read_cert()->public_key().encrypt(key.serialize()),
          symmetric_encrypt::encrypt(key, s.get_buffer(), s.size()),
          *writter.private_key()));
    }

    template<typename item_type>
    void add_log(
      transactions::transaction_block_builder & log,
      item_type && item) {

      binary_serializer s;
      s
        << (uint8_t)item_type::message_id;
      item.visit(_serialize_visitor(s));

      this->add_to_log(log, s.get_buffer(), s.size());
    }

  private:
    friend class user_channel;

		const_data_buffer id_;
    user_channel::channel_type_t channel_type_;
	  std::string name_;

    std::map<std::string, std::shared_ptr<certificate>> read_certificates_;
    std::map<std::string, std::shared_ptr<certificate>> write_certificates_;

    std::map<std::string, std::shared_ptr<asymmetric_private_key>> read_private_keys_;
    std::map<std::string, std::shared_ptr<asymmetric_private_key>> write_private_keys_;

    std::string current_read_certificate_;
    std::string current_write_certificate_;

    void add_to_log(
        transactions::transaction_block_builder & log,
        const uint8_t * data,
        size_t size);
  };
}

#endif // __VDS_USER_MANAGER_USER_CHANNEL_P_H_
