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
#include "create_user_transaction.h"

vds::user_channel::user_channel()
: impl_(nullptr) {
}

vds::user_channel::user_channel(user_channel&& other)
: impl_(other.impl_) {
  other.impl_ = nullptr;
}

vds::user_channel::user_channel(
  const const_data_buffer & id,
  const std::string & channel_type,
  const std::string & name,
  const const_data_buffer & read_id,
  const std::shared_ptr<asymmetric_public_key> & read_cert,
  const std::shared_ptr<asymmetric_private_key> & read_private_key,
  const const_data_buffer & write_id,
  const std::shared_ptr<asymmetric_public_key> & write_cert,
  const std::shared_ptr<asymmetric_private_key> & write_private_key)
: impl_(new _user_channel(id, channel_type, name, read_id, read_cert, read_private_key, write_id, write_cert, write_private_key))
{
}

vds::user_channel::~user_channel() {
  delete this->impl_;
}

const vds::const_data_buffer &vds::user_channel::id() const {
  return this->impl_->id();
}

const std::string& vds::user_channel::channel_type() const {
  return this->impl_->channel_type();
}

const std::string& vds::user_channel::name() const {
  return this->impl_->name();
}

vds::expected<std::shared_ptr<vds::asymmetric_public_key>> vds::user_channel::read_public_key() const {
  return this->impl_->read_cert();
}

vds::expected<std::shared_ptr<vds::asymmetric_public_key>> vds::user_channel::write_public_key() const {
  return this->impl_->write_cert();
}

vds::expected<void> vds::user_channel::add_reader(
	transactions::transaction_block_builder& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& /*channel_read_private_key*/) const
{
	return this->impl_->add_reader(playback, member_user, owner_user, owner_private_key);
}

vds::expected<void> vds::user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user) const
{
	return this->impl_->add_writer(playback, member_user, owner_user);
}

std::shared_ptr<vds::asymmetric_private_key> vds::user_channel::read_cert_private_key(const const_data_buffer& cert_subject) {
  return this->impl_->read_cert_private_key(cert_subject);
}

vds::user_channel& vds::user_channel::operator=(user_channel&& other) {
  delete this->impl_;
  this->impl_ = other.impl_;
  other.impl_ = nullptr;
  return *this;
}

vds::expected<void> vds::user_channel::add_to_log(
    transactions::transaction_block_builder& log,
    const uint8_t* data, size_t size) {
  return this->impl_->add_to_log(log, data, size);
}

/////////////////////////////////////////////////
vds::_user_channel::_user_channel(
  const const_data_buffer &id,
  const std::string & channel_type,
	const std::string & name,
  const const_data_buffer & read_id,
	const std::shared_ptr<asymmetric_public_key> & read_cert,
  const std::shared_ptr<asymmetric_private_key> & read_private_key,
  const const_data_buffer & write_id,
  const std::shared_ptr<asymmetric_public_key> & write_cert,
  const std::shared_ptr<asymmetric_private_key> & write_private_key)
: id_(id), channel_type_(channel_type),  name_(name)
{
  this->read_certificates_[read_id] = read_cert;
  this->read_private_keys_[read_id] = read_private_key;
  this->current_read_certificate_ = read_id;

  this->write_certificates_[write_id] = write_cert;
  this->write_private_keys_[write_id] = write_private_key;
  this->current_write_certificate_ = write_id;
}

vds::expected<void> vds::_user_channel::add_reader(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user,
  const asymmetric_private_key& /*owner_private_key*/) const
{
  if(!this->current_read_certificate_ || !this->current_write_certificate_) {
    return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::add_reader");
  }

  GET_EXPECTED(pc, member_user->personal_channel());
	return pc->add_log(
    playback,
    owner_user,
    message_create<transactions::channel_add_reader_transaction>(
      this->id_,
      this->channel_type_,
      this->name_,
      this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
      this->write_certificates_.find(this->current_write_certificate_)->second));
}

vds::expected<void> vds::_user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const member_user& member_user,
  const vds::member_user& owner_user) const
{
  if (!this->current_read_certificate_ || !this->current_write_certificate_) {
    return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::add_reader");
  }

  GET_EXPECTED(pc, member_user->personal_channel());
  return pc->add_log(
    playback,
    owner_user,
    message_create<transactions::channel_add_writer_transaction>(
      this->id_,
      this->channel_type_,
      this->name_,
      this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
      this->write_certificates_.find(this->current_write_certificate_)->second,
      this->write_private_keys_.find(this->current_write_certificate_)->second));
}


vds::expected<void> vds::_user_channel::add_writer(
  transactions::transaction_block_builder& playback,
  const std::string & name,
  const vds::member_user& member_user,
  const vds::member_user& owner_user) const
{
  if (!this->current_read_certificate_ || !this->current_write_certificate_) {
    return vds::make_unexpected<std::invalid_argument>("vds::_user_channel::add_reader");
  }
  
  GET_EXPECTED(pc, member_user->personal_channel());
  return pc->add_log(
    playback,
    owner_user,
    message_create<transactions::channel_add_writer_transaction>(
      this->id_,
      this->channel_type_,
      name,
      this->read_certificates_.find(this->current_read_certificate_)->second,
      this->read_private_keys_.find(this->current_read_certificate_)->second,
      this->write_certificates_.find(this->current_write_certificate_)->second,
      this->write_private_keys_.find(this->current_write_certificate_)->second));
}

vds::expected<std::shared_ptr<vds::user_channel>> vds::_user_channel::import_personal_channel(
  const std::shared_ptr<asymmetric_public_key>& user_cert,
  const std::shared_ptr<asymmetric_private_key>& user_private_key) {

  GET_EXPECTED(fp, user_cert->hash(hash::sha256()));

  return std::make_shared<user_channel>(
    fp,
    user_channel::channel_type_t::personal_channel,
    "!Private",
    fp,
    user_cert,
    user_private_key,
    fp,
    user_cert,
    user_private_key);
}

vds::expected<void> vds::_user_channel::add_to_log(
    transactions::transaction_block_builder &log,
    const uint8_t * data, size_t size) {

  auto key = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  GET_EXPECTED(write_cert, this->write_cert());
  GET_EXPECTED(read_cert, this->read_cert());
  GET_EXPECTED(key_data, key.serialize());
  GET_EXPECTED(read_id, read_cert->hash(hash::sha256()));
  GET_EXPECTED(write_id, write_cert->hash(hash::sha256()));
  GET_EXPECTED(read_cert_public_key_data, read_cert->encrypt(key_data));
  GET_EXPECTED(write_private_key, this->write_private_key());
  GET_EXPECTED(key_crypted, symmetric_encrypt::encrypt(key, data, size));

  return log.add(transactions::channel_message::create(
    this->id_,
    read_id,
    write_id,
    read_cert_public_key_data,
    key_crypted,
    *write_private_key));
}
