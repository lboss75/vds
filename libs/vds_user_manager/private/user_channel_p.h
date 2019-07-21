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
      _user_channel(
        const const_data_buffer &id,
        const std::string & channel_type,
		    const std::string & name,
        const const_data_buffer & read_id,
        const std::shared_ptr<asymmetric_public_key> & read_cert,
        const std::shared_ptr<asymmetric_private_key> & read_private_key,
        const const_data_buffer & write_id, 
        const std::shared_ptr<asymmetric_public_key> & write_cert,
        const std::shared_ptr<asymmetric_private_key> & write_private_key);

    const const_data_buffer &id() const { return this->id_;}
    const std::string & channel_type() const { return this->channel_type_; }
    const std::string &name() const { return this->name_; }

    expected<std::shared_ptr<vds::asymmetric_public_key>> read_cert() const {
      if (!this->current_read_certificate_) {
        return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::add_reader");
      }
      return this->read_certificates_.find(this->current_read_certificate_)->second;
    }

    std::shared_ptr<asymmetric_public_key> read_cert(const const_data_buffer & subject) const {
      auto p = this->read_certificates_.find(subject);
      if (this->read_certificates_.end() == p) {
        return std::shared_ptr<asymmetric_public_key>();
      }

      return p->second;
    }

    expected<std::shared_ptr<asymmetric_public_key>> write_cert() const {
      if (!this->current_read_certificate_ || !this->current_write_certificate_) {
        return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::add_reader");
      }
      return this->write_certificates_.find(this->current_write_certificate_)->second;
    }

    expected<std::shared_ptr<asymmetric_private_key>> read_private_key() const {
      if (!this->current_read_certificate_) {
        return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::read_private_key");
      }
      return this->read_private_keys_.find(this->current_read_certificate_)->second;
    }

    expected<std::shared_ptr<asymmetric_private_key>> write_private_key() const {
      if (!this->current_write_certificate_) {
        return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::write_private_key");
      }
      return this->write_private_keys_.find(this->current_write_certificate_)->second;
    }

    expected<void> add_reader(
	    transactions::transaction_block_builder& playback,
	    const member_user& member_user,
	    const vds::member_user& owner_user,
	    const asymmetric_private_key& owner_private_key) const;

    expected<void> add_writer(
	    transactions::transaction_block_builder& playback,
	    const member_user& member_user,
	    const vds::member_user& owner_user) const;

    expected<void> add_writer(
      transactions::transaction_block_builder& playback,
      const std::string & name,
      const member_user& member_user,
      const vds::member_user& owner_user) const;

    std::shared_ptr<asymmetric_private_key> read_cert_private_key(const const_data_buffer & cert_subject) {
      auto p = this->read_private_keys_.find(cert_subject);
      if (this->read_private_keys_.end() == p) {
        return std::shared_ptr<asymmetric_private_key>();
      }

      return p->second;
    }

    static expected<std::shared_ptr <user_channel>> import_personal_channel(
      
      const std::shared_ptr<asymmetric_public_key> & user_cert,
      const std::shared_ptr<asymmetric_private_key> & user_private_key);

    template<typename item_type>
    expected<void> add_log(
      transactions::transaction_block_builder & log,
      const member_user & writter,
      expected<item_type> && item) {

      CHECK_EXPECTED_ERROR(item);

      auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

      binary_serializer s;
      CHECK_EXPECTED(s << (uint8_t)item_type::message_id);

      _serialize_visitor v(s);
      item.value().visit(v);
      if(v.error()) {
        return unexpected(std::move(v.error()));
      }

      GET_EXPECTED(read_cert, this->read_cert());
      GET_EXPECTED(read_id, read_cert->fingerprint());
      GET_EXPECTED(key_crypted, read_cert->encrypt(key.serialize()));
      GET_EXPECTED(s_crypted, symmetric_encrypt::encrypt(key, s.get_buffer(), s.size()));
      GET_EXPECTED(write_id, writter.user_public_key()->fingerprint());

      return log.add(
          transactions::channel_message::create(
            this->id_,
            read_id,
            write_id,
            key_crypted,
            s_crypted,
            *writter.private_key()));
    }

    template<typename item_type>
    expected<void> add_log(
      transactions::transaction_block_builder & log,
      expected<item_type> && item) {

      CHECK_EXPECTED_ERROR(item);

      binary_serializer s;
      CHECK_EXPECTED(s << (uint8_t)item_type::message_id);
      _serialize_visitor v(s);
      item.value().visit(v);

      return this->add_to_log(log, s.get_buffer(), s.size());
    }

  private:
    friend class user_channel;

		const_data_buffer id_;
    std::string channel_type_;
	  std::string name_;

    std::map<const_data_buffer, std::shared_ptr<asymmetric_public_key>> read_certificates_;
    std::map<const_data_buffer, std::shared_ptr<asymmetric_public_key>> write_certificates_;

    std::map<const_data_buffer, std::shared_ptr<asymmetric_private_key>> read_private_keys_;
    std::map<const_data_buffer, std::shared_ptr<asymmetric_private_key>> write_private_keys_;

    const_data_buffer current_read_certificate_;
    const_data_buffer current_write_certificate_;

    expected<void> add_to_log(
        transactions::transaction_block_builder & log,
        const uint8_t * data,
        size_t size);
  };
}

#endif // __VDS_USER_MANAGER_USER_CHANNEL_P_H_
