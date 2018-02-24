/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
#include "stdafx.h"
#include "user_channel.h"
#include "private/user_channel_p.h"
#include "transactions/channel_add_reader_transaction.h"
#include "transactions/channel_add_writer_transaction.h"

vds::user_channel::user_channel() {
}

vds::user_channel::user_channel(
    const vds::guid &id,
	const std::string & name,
	const vds::certificate & read_cert,
    const vds::certificate & write_cert)
: impl_(new _user_channel(id, name, read_cert, write_cert))
{
}

const vds::guid &vds::user_channel::id() const {
  return this->impl_->id();
}

const vds::certificate &vds::user_channel::read_cert() const {
  return this->impl_->read_cert();
}

const vds::certificate &vds::user_channel::write_cert() const {
  return this->impl_->write_cert();
}

void vds::user_channel::add_reader(
	transactions::transaction_block& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& channel_read_private_key) const
{
	this->impl_->add_reader(playback, member_user, owner_user, owner_private_key, channel_read_private_key);
}

void vds::user_channel::add_writer(
	transactions::transaction_block& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& channel_write_private_key) const
{
	this->impl_->add_writer(playback, member_user, owner_user, owner_private_key, channel_write_private_key);
}

/////////////////////////////////////////////////
vds::_user_channel::_user_channel(
    const vds::guid &id,
	const std::string & name,
	const vds::certificate & read_cert,
    const vds::certificate & write_cert)
: id_(id), name_(name), read_cert_(read_cert), write_cert_(write_cert)
{
}

void vds::_user_channel::add_reader(
	transactions::transaction_block& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& channel_read_private_key) const
{
	playback.add(
    member_user.id(),
		transactions::channel_add_reader_transaction(
			this->id_,
			this->name_,
			this->read_cert_,
			channel_read_private_key,
			this->write_cert_));
}

void vds::_user_channel::add_writer(
	transactions::transaction_block& playback,
	const member_user& member_user,
	const vds::member_user& owner_user,
	const asymmetric_private_key& owner_private_key,
	const asymmetric_private_key& channel_write_private_key) const
{
	playback.add(
		member_user.id(),
		transactions::channel_add_writer_transaction(
			this->id_,
			this->name_,
			this->read_cert_,
			this->write_cert_,
			channel_write_private_key));
}
