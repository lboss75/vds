/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "device_activation.h"
#include "private/member_user_p.h"



vds::const_data_buffer vds::device_activation::pack(const std::string& user_password) const
{
  return  (
    binary_serializer()
    << this->user_name_
    << this->certificate_chain_
    << this->private_key_.der(user_password)
    << asymmetric_sign::signature(
      hash::sha256(),
      this->private_key_,
      (binary_serializer()
        << this->user_name_
        << this->certificate_chain_
        << this->private_key_.der(user_password)
        << hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data())).data();
}

vds::device_activation vds::device_activation::unpack(const const_data_buffer& data, const std::string& user_password)
{
  std::string user_name;
  const_data_buffer user_key_data;

  std::list<certificate> certificate_chain;
  const_data_buffer signature;

  binary_deserializer s(data);
  s
    >> user_name
    >> certificate_chain
    >> user_key_data
    >> signature;

  if (!asymmetric_sign_verify::verify(hash::sha256(), certificate_chain.rbegin()->public_key(), signature,
    (binary_serializer()
      << user_name
      << certificate_chain
      << user_key_data
      << hash::signature(hash::md5(), user_password.c_str(), user_password.length())).data()))
  {
    throw std::runtime_error("Signature error");
  }

  const auto private_key = asymmetric_private_key::parse_der(user_key_data, user_password);

  return device_activation(
    user_name,
    certificate_chain,
    private_key);
}
