#include "stdafx.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

/*
 * User: user_id -> certificate (id, user_id, parent_id)
 *
 *
 */

static vds::crypto_service::certificate_extension_type id_extension_type()
{
  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
      "1.2.3.4",
      "VDS Identifier",
      "VDS Identifier");

  return result;
}

static vds::crypto_service::certificate_extension_type parent_id_extension_type()
{
  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
      "1.2.3.5",
      "VDS Parent Identifier",
      "VDS Parent Identifier");

  return result;
}

static vds::crypto_service::certificate_extension_type user_id_extension_type()
{
  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
      "1.2.3.6",
      "VDS User Identifier",
      "VDS User Identifier");

  return result;
}

static vds::crypto_service::certificate_extension_type parent_user_id_extension_type()
{
	static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
		"1.2.3.7",
		"Parent VDS User Identifier",
		"Parent VDS User Identifier");

	return result;
}

static vds::guid certificate_parent_id(const vds::certificate & cert)
{
  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value);
}

vds::certificate vds::_cert_control::create_root(
    const guid &user_id,
    const std::string &name,
    const vds::asymmetric_private_key &private_key) {

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = name;

  local_user_options.extensions.push_back(
      certificate_extension(id_extension_type(), guid::new_guid().str()));

  local_user_options.extensions.push_back(
      certificate_extension(user_id_extension_type(), user_id.str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_user_cert(
    const guid & id,
	const guid & user_id,
	const std::string & name,
    const vds::asymmetric_private_key & private_key,
    const certificate & user_cert,
    const asymmetric_private_key & user_private_key) {
  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = name;
  local_user_options.ca_certificate = &user_cert;
  local_user_options.ca_certificate_private_key = &user_private_key;

  local_user_options.extensions.push_back(
      certificate_extension(id_extension_type(), id.str()));

  local_user_options.extensions.push_back(
      certificate_extension(user_id_extension_type(), user_id.str()));

  local_user_options.extensions.push_back(
      certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

  local_user_options.extensions.push_back(
	  certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_cert(
	const guid & id,
	const std::string & name,
	const vds::asymmetric_private_key & private_key,
	const certificate & user_cert,
	const asymmetric_private_key & user_private_key) {
	certificate::create_options local_user_options;
	local_user_options.country = "RU";
	local_user_options.organization = "IVySoft";
	local_user_options.name = name;
	local_user_options.ca_certificate = &user_cert;
	local_user_options.ca_certificate_private_key = &user_private_key;

	local_user_options.extensions.push_back(
		certificate_extension(id_extension_type(), id.str()));

	local_user_options.extensions.push_back(
		certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

	local_user_options.extensions.push_back(
		certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

	asymmetric_public_key cert_pkey(private_key);
	return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::guid vds::cert_control::get_id(const vds::certificate &cert) {
  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(id_extension_type())).value);
}

vds::guid vds::cert_control::get_user_id(const vds::certificate &cert) {
  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(user_id_extension_type())).value);
}

vds::guid vds::cert_control::get_parent_id(const vds::certificate &cert) {
  auto parent = cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value;
  if(parent.empty()){
    return vds::guid();
  }
  else {
    return vds::guid::parse(parent);
  }
}

vds::guid vds::cert_control::get_parent_user_id(const vds::certificate &cert) {
	auto parent = cert.get_extension(cert.extension_by_NID(parent_user_id_extension_type())).value;
	if (parent.empty()) {
		return vds::guid();
	}
	else {
		return vds::guid::parse(parent);
	}
}

