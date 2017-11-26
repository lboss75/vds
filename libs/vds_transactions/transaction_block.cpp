/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_block.h"
#include "symmetriccrypto.h"
#include "asymmetriccrypto.h"
#include "guid.h"
#include "transaction_context.h"

vds::const_data_buffer
vds::transaction_block::sign(const guid &target_cert_id, const class certificate &target_cert,
                             const class guid &sign_cert_key_id,
                             const class asymmetric_private_key &sign_cert_key) {

  auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  binary_serializer result;
  result
      << sign_cert_key_id
      << target_cert_id
      << target_cert.public_key().encrypt(skey.serialize())
      << symmetric_encrypt::encrypt(skey, this->s_.data());

  result << asymmetric_sign::signature(
      hash::sha256(),
      sign_cert_key,
      result.data());

  return result.data();
}

vds::const_data_buffer vds::transaction_block::unpack_block(
    service_provider & sp,
    const vds::const_data_buffer &data,
    const std::function<certificate(const guid &)> &get_cert_handler,
    const std::function<asymmetric_private_key(const guid &)> & get_key_handler) {
  binary_deserializer s(data);

  guid sign_cert_id;
  guid target_cert_id;
  const_data_buffer skey_data;
  const_data_buffer crypted_data;
  s >> sign_cert_id >> target_cert_id >> skey_data >> crypted_data;

  auto body_size = data.size() - s.size();

  const_data_buffer sign;
  s >> sign;

  if(0 != s.size()){
    throw std::runtime_error("Invalid data");
  }

  auto cert = get_cert_handler(sign_cert_id);

  if(!cert){
    throw std::runtime_error("Certificate " + sign_cert_id.str() + " is not found");
  }

  if(!asymmetric_sign_verify::verify(
      hash::sha256(),
      cert.public_key(),
      sign,
      data.data(), body_size)){
    throw std::runtime_error("Invalid data");
  }

  auto key = get_key_handler(target_cert_id);

  if(!key){
    throw std::runtime_error("Key " + target_cert_id.str() + " is not found");
  }

  auto decrypted = key.decrypt(skey_data);
  auto skey = symmetric_key::deserialize(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(decrypted));

  sp.set_property(
      service_provider::property_scope::local_scope,
      new transaction_context(sign_cert_id));

  return symmetric_decrypt::decrypt(skey, crypted_data);
}
