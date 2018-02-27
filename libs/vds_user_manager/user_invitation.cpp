/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "user_invitation.h"
#include "private/member_user_p.h"


vds::member_user vds::user_invitation::get_user() const
{
	return member_user(new _member_user(this->user_id_, this->user_certificate_));
}

vds::const_data_buffer vds::user_invitation::pack(const std::string& user_password) const
{
	return  (
		binary_serializer()
		<< this->user_id_
		<< this->user_name_
		<< this->user_certificate_
		<< this->user_private_key_.der(user_password)
		<< this->certificate_chain_
		<< asymmetric_sign::signature(
			hash::sha256(), 
			this->user_private_key_,
			(binary_serializer()
				<< this->user_id_
				<< this->user_name_
				<< this->user_certificate_
				<< this->user_private_key_.der(user_password)
				<< this->certificate_chain_
				<< hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data())).data();
}

vds::user_invitation vds::user_invitation::unpack(const const_data_buffer& data, const std::string& user_password)
{
	guid user_id;
	std::string user_name;
	certificate user_cert;
	const_data_buffer user_key_data;

	std::list<certificate> certificate_chain;
	const_data_buffer signature;

	binary_deserializer s(data);
	s
		>> user_id
		>> user_name
		>> user_cert
		>> user_key_data
		>> certificate_chain
		>> signature;

	if (!asymmetric_sign_verify::verify(hash::sha256(), user_cert.public_key(), signature,
		(binary_serializer()
			<< user_id
			<< user_name
			<< user_cert
			<< user_key_data
			<< certificate_chain
			<< hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data()))
	{
		throw std::runtime_error("Signature error");
	}

	const auto user_private_key = asymmetric_private_key::parse_der(user_key_data, user_password);

	return user_invitation(
		user_id,
		user_name,
		user_cert,
		asymmetric_private_key::parse_der(user_key_data, user_password),
		certificate_chain);
}
