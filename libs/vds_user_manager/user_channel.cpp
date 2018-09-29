/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_channel.h"
#include "private/user_channel_p.h"
#include "channel_add_reader_transaction.h"
#include "channel_add_writer_transaction.h"
#include "private/member_user_p.h"

vds::user_channel::user_channel()
: impl_(nullptr) {
}

vds::user_channel::user_channel(
  const const_data_buffer & id,
  channel_type_t channel_type,
  const std::string & name,
  const std::shared_ptr<certificate> & read_cert,
  const std::shared_ptr<asymmetric_private_key> & read_private_key,
  const std::shared_ptr<certificate> & write_cert,
  const std::shared_ptr<asymmetric_private_key> & write_private_key)
: impl_(new _user_channel(id, channel_type, name, read_cert, read_private_key, write_cert, write_private_key))
{
}

vds::user_channel::~user_channel() {
  delete this->impl_;
}

const vds::const_data_buffer &vds::user_channel::id() const {
  return this->impl_->id();
}

const std::string& vds::user_channel::name() const {
  return this->impl_->name();
}

const std::shared_ptr<vds::certificate> &vds::user_channel::read_cert() const {
  return this->impl_->read_cert();
}

const std::shared_ptr<vds::certificate> &vds::user_channel::write_cert() const {
  return this->impl_->write_cert();
}

void vds::user_channel::add_reader(
	transactions::transaction_block_builder& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& channel_read_private_key) const
{
	this->impl_->add_reader(playback, member_user, owner_user, owner_private_key);
}

void vds::user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user) const
{
	this->impl_->add_writer(playback, member_user, owner_user);
}

std::shared_ptr<vds::asymmetric_private_key> vds::user_channel::read_cert_private_key(const std::string& cert_subject) {
  return this->impl_->read_cert_private_key(cert_subject);
}

void vds::user_channel::add_to_log(
    transactions::transaction_block_builder& log,
    const uint8_t* data, size_t size) {
  this->impl_->add_to_log(log, data, size);
}

/////////////////////////////////////////////////
vds::_user_channel::_user_channel(
  const const_data_buffer &id,
  user_channel::channel_type_t channel_type,
	const std::string & name,
	const std::shared_ptr<certificate> & read_cert,
  const std::shared_ptr<asymmetric_private_key> & read_private_key,
  const std::shared_ptr<certificate> & write_cert,
  const std::shared_ptr<asymmetric_private_key> & write_private_key)
: id_(id), channel_type_(channel_type),  name_(name)
{
  auto read_id = read_cert->subject();
  this->read_certificates_[read_id] = read_cert;
  this->read_private_keys_[read_id] = read_private_key;
  this->current_read_certificate_ = read_id;

  auto write_id = write_cert->subject();
  this->write_certificates_[write_id] = write_cert;
  this->write_private_keys_[write_id] = write_private_key;
  this->current_write_certificate_ = write_id;
}

void vds::_user_channel::add_reader(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user,
  const asymmetric_private_key& owner_private_key) const
{
  if(this->current_read_certificate_.empty() || this->current_write_certificate_.empty()) {
    throw std::invalid_argument("vds::_user_channel::add_reader");
  }

	member_user->personal_channel()->add_log(
    playback,
    owner_user,
    transactions::channel_add_reader_transaction(
      this->id_,
      std::to_string(this->channel_type_),
			this->name_,
			this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
		  this->write_certificates_.find(this->current_write_certificate_)->second));
}

void vds::_user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user) const
{
  if (this->current_read_certificate_.empty() || this->current_write_certificate_.empty()) {
    throw std::invalid_argument("vds::_user_channel::add_reader");
  }
  
  member_user->personal_channel()->add_log(
    playback,
    owner_user,
		transactions::channel_add_writer_transaction(
      this->id_,
      std::to_string(this->channel_type_),
      this->name_,
      this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
      this->write_certificates_.find(this->current_write_certificate_)->second,
      this->write_private_keys_.find(this->current_write_certificate_)->second));
}


void vds::_user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const std::string & name,
  const member_user& member_user,
  const vds::member_user& owner_user) const
{
  if (this->current_read_certificate_.empty() || this->current_write_certificate_.empty()) {
    throw std::invalid_argument("vds::_user_channel::add_reader");
  }

  member_user->personal_channel()->add_log(
    playback,
    owner_user,
    transactions::channel_add_writer_transaction(
      this->id_,
      std::to_string(this->channel_type_),
      name,
      this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
      this->write_certificates_.find(this->current_write_certificate_)->second,
      this->write_private_keys_.find(this->current_write_certificate_)->second));
}

vds::user_channel vds::_user_channel::import_personal_channel(
  const service_provider& sp,
  const std::shared_ptr<certificate>& user_cert,
  const std::shared_ptr<asymmetric_private_key>& user_private_key) {

  return user_channel(
    user_cert->fingerprint(),
    user_channel::channel_type_t::personal_channel,
    "!Private",
    user_cert,
    user_private_key,
    user_cert,
    user_private_key);
}

void vds::_user_channel::add_to_log(
    transactions::transaction_block_builder &log,
    const uint8_t * data, size_t size) {

  auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  log.add(
      transactions::channel_message(
          this->id_,
          this->read_cert()->subject(),
          this->write_cert()->subject(),
          this->read_cert()->public_key().encrypt(key.serialize()),
          symmetric_encrypt::encrypt(key, data, size),
          *this->write_private_key()));
}
