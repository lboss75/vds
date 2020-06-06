#include "stdafx.h"
#include "private/cert_control_p.h"
#include "keys_control.h"

char vds::keys_control::root_id_[65] =
"9L4hUK/qkThWrhyWsl1NC/8MLYZtOqdJFZ2Y1uIDsr0=";

vds::expected<vds::certificate> vds::_cert_control::create_cert(
  const vds::asymmetric_private_key & private_key) {
  
  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
  GET_EXPECTED(name, cert_pkey.fingerprint());

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = base64::from_bytes(name);

  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::expected<vds::certificate> vds::_cert_control::create_cert(
	const vds::asymmetric_private_key & private_key,
	const certificate & user_cert,
	const asymmetric_private_key & user_private_key) {
  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
  GET_EXPECTED(name, cert_pkey.fingerprint());
  
  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = base64::from_bytes(name);
  local_user_options.ca_certificate = &user_cert;
  local_user_options.ca_certificate_private_key = &user_private_key;

  return certificate::create_new(cert_pkey, private_key, local_user_options);
}


vds::expected<void> vds::keys_control::private_info_t::genereate_all() {
  GET_EXPECTED(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->root_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));
 

  return expected<void>();
}

static void save_buffer(char(&buffer_storage)[65], const vds::const_data_buffer & data) {
  auto storage_str = vds::base64::from_bytes(data);
  vds_assert(sizeof(buffer_storage) > storage_str.length());
  strcpy(buffer_storage, storage_str.c_str());
}

static vds::expected<void> save_public_key(char (&public_key_storage)[vds::asymmetric_public_key::base64_size + 1], const vds::asymmetric_public_key & public_key) {
  auto der = public_key.der();
  if(der.has_error()) {
    return vds::make_unexpected<std::runtime_error>(der.error()->what());
  }

  auto cert_storage_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(public_key_storage) > cert_storage_str.length());
  strcpy(public_key_storage, cert_storage_str.c_str());

  return vds::expected<void>();
}

static vds::expected<void> save_private_key(char (&private_key_storage)[vds::asymmetric_private_key::base64_size + 1], const vds::asymmetric_private_key & private_key) {
  auto der = private_key.der(std::string());
  if (der.has_error()) {
    return vds::make_unexpected<std::runtime_error>(der.error()->what());
  }

  const auto private_key_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(private_key_storage) > private_key_str.length());
  strcpy(private_key_storage, private_key_str.c_str());

  return vds::expected<void>();
}

vds::expected<void> vds::keys_control::genereate_all(
  const private_info_t & private_info) {
  GET_EXPECTED(root_certificate, asymmetric_public_key::create(*private_info.root_private_key_));
  GET_EXPECTED(root_id, root_certificate.fingerprint());
  save_buffer(root_id_, root_id);

  return expected<void>();
}
