/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "transaction_block.h"
#include "symmetriccrypto.h"
#include "asymmetriccrypto.h"
#include "guid.h"

vds::const_data_buffer
vds::transaction_block::sign(
  const guid & cert_id,
  const certificate & cert,
  const asymmetric_private_key & cert_key) {

  auto skey = symmetric_key::generate(symmetric_crypto::aes_256_cbc());

  binary_serializer result;
  result
      << cert_id
      << cert.public_key().encrypt(skey.serialize())
      << symmetric_encrypt::encrypt(skey, this->s_.data());

  result << asymmetric_sign::signature(
      hash::sha256(),
      cert_key,
      result.data());

  return result.data();
}

vds::const_data_buffer vds::transaction_block::unpack_block(
    const vds::const_data_buffer &data,
    const std::function<void(
        const vds::guid &,
        vds::certificate &,
        vds::asymmetric_private_key &)> &get_cert_handler) {
  binary_deserializer s(data);

  guid cert_id;
  const_data_buffer skey_data;
  const_data_buffer crypted_data;
  s >> cert_id >> skey_data >> crypted_data;

  auto body_size = data.size() - s.size();

  const_data_buffer sign;
  s >> sign;

  if(0 != data.size()){
    throw std::runtime_error("Invalid data");
  }

  certificate cert;
  asymmetric_private_key key;

  get_cert_handler(cert_id, cert, key);

  if(!asymmetric_sign_verify::verify(
      hash::sha256(),
      cert.public_key(),
      sign,
      data.data(), body_size)){
    throw std::runtime_error("Invalid data");
  }

  auto decrypted = key.decrypt(skey_data);
  auto skey = symmetric_key::deserialize(
      symmetric_crypto::aes_256_cbc(),
      binary_deserializer(decrypted));

  return symmetric_decrypt::decrypt(skey, crypted_data);
}
