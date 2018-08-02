#include "stdafx.h"
#include "private/cert_control_p.h"
#include "cert_control.h"

char vds::cert_control::root_certificate_[1821] =
"MIIFTzCCAzegAwIBAgIBATANBgkqhkiG9w0BAQsFADA6MQswCQYDVQQGEwJSVTEQMA4GA1UECgwHSVZ5"
"U29mdDEZMBcGA1UEAwwQdmFkaW1AaXYtc29mdC5ydTAeFw0xODA4MDIxMjI4MjZaFw0xOTA4MDIxMjI4"
"MjZaMDoxCzAJBgNVBAYTAlJVMRAwDgYDVQQKDAdJVnlTb2Z0MRkwFwYDVQQDDBB2YWRpbUBpdi1zb2Z0"
"LnJ1MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAwppm+Upf9suFXNoKvFhfSZpP5suCctDA"
"zQ4Kw0xil3BbxUnv5sG/v0Jybz1kgn0J7Er6JOyDd4Ovsh1mTzhj1PxZaW5Cxe7ntZVI7PLBBlIScVAn"
"t91Z2yaHJqmStSOj0wfOfaiNiOmeqcF1xpHKX/CQdioG3PG9qj6e47Hm3EJLy9wsCCUtRgQMO8+Z5LW0"
"5YijjWENIHljZh1mAzxc9HD5/u6Q3s89S4yU/zaXX1hDXdELDdvww5PEj0PQqdlxXJeFWgcRkee2VrC/"
"JBUPbaZxBJjoeIequm3UIDhFZsv20Tbdx18AJ/KhHCqcVTU+SIUL5ZjTSpIcrqM47kXcIRqmIZGGSD++"
"jMP/TV2l+YdhRS3AKdCd0Qr5bU/j/BmTueKHI+0nTpzDucPMbrt/mQBUzehhi+J+BtYAmoUk3pVP7rpO"
"Ocb51hgtYeWXFOXbysNJPihbjRmvVxRecefIJbua0U2k7wXF5okGsOLJlIRQ0Bm6B1C4n/VAZGACAEk7"
"d2CoHk3c4CPK2d39aC/GVonAHzCaw577kKSjseioRmpeGDSSyMjI52KkihY5wjLsBYaSjE0quBVwBe0E"
"uo47mD18hzX2s2YntE2WDB2s2EufA9/x9KPd8G2MosBdwDfCfqfqSIgD5Hq3LJzLQvojtvUo6cJyq8pj"
"IHJbq/hjI8MCAwEAAaNgMF4wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwEQYJYIZIAYb4"
"QgEBBAQDAgIEMCgGCWCGSAGG+EIBDQQbFhlleGFtcGxlIGNvbW1lbnQgZXh0ZW5zaW9uMA0GCSqGSIb3"
"DQEBCwUAA4ICAQB5HesBzm21Jd7wXjFlFlKAjPykgb6rZpaGbXHu2uvDG/bcHc6M6YJlhgy42yyxtNV1"
"g2Vgiy89RvPqO9TjmcTYDGigpvv4FfxO+rtVey/vjfLZk/9ayeGnQPnvnZ8alSmxnw6gO8wOaL93aeUu"
"6+wM+ngtMlyImzFgL95NsXuE7WHN8wMp0NDFbzh/tOyXUdXE2KY/Obx5rzawD+4JCLLWVPdGYoeY18hd"
"AgfEQjQRfozbyeSR5YMfcK5ddgZhS9fRLUfpbtdBF2R/KRy5pihGIHzBBtZ1DNYO7dVDOd2CEmfk528B"
"3S25gZm6ZSGWffp4kyusLd+Y+HuRA8CRjexWdPsrjfvGrjxf8s3Y3kLLYk9KxGOkJaB99ZgWAOEpVbXo"
"P5+LN6aGon3DyEZe2zz1rNQ2X/fS++2UoyH3uTo8kIjhaCTAl5eRIzTizulRmlYlapWeOBVV5Ck8X7PS"
"CFORwqafKB5JBwWnq8Xv60vCfp5TarSdKf0K9WWL478nNp5BB5njKNAAp5JdBXEekxfpwcR/qMgS6lNz"
"wfmZNiJbE7J6UdJRx7LlU/Wbf+xoUCVspFIM9kQoNq127So1JglcpR8NlVryooIDSNv+SKcBhk43GJv8"
"09UGOdvYql2AJQeXo6wvKBur5QTaqFlKasrNqpayUWSD61LS8S+TDFZNgQ==";

char vds::cert_control::root_private_key_[3133] =
"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"
"kBllr/Ngo5sP";

/*
 * User: user_id -> certificate (object_id, user_id, parent_id)
 *
 *
 */

//static vds::crypto_service::certificate_extension_type id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.4",
//      "VDS Identifier",
//      "VDS Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type parent_id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.5",
//      "VDS Parent Identifier",
//      "VDS Parent Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type user_id_extension_type()
//{
//  static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//      "1.2.3.6",
//      "VDS User Identifier",
//      "VDS User Identifier");
//
//  return result;
//}
//
//static vds::crypto_service::certificate_extension_type parent_user_id_extension_type()
//{
//	static vds::crypto_service::certificate_extension_type result = vds::crypto_service::register_certificate_extension_type(
//		"1.2.3.7",
//		"Parent VDS User Identifier",
//		"Parent VDS User Identifier");
//
//	return result;
//}
//
//static vds::guid certificate_parent_id(const vds::certificate & cert)
//{
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value);
//}

vds::certificate vds::_cert_control::create_root(
    const std::string &name,
    const vds::asymmetric_private_key &private_key) {

  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = name;

  //local_user_options.extensions.push_back(
  //    certificate_extension(id_extension_type(), guid::new_guid().str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(user_id_extension_type(), user_id.str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_user_cert(
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

  //local_user_options.extensions.push_back(
  //    certificate_extension(id_extension_type(), object_id.str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(user_id_extension_type(), user_id.str()));

  //local_user_options.extensions.push_back(
  //    certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

  //local_user_options.extensions.push_back(
	 // certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

  asymmetric_public_key cert_pkey(private_key);
  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

vds::certificate vds::_cert_control::create_cert(
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

	//local_user_options.extensions.push_back(
	//	certificate_extension(id_extension_type(), object_id.str()));

	//local_user_options.extensions.push_back(
	//	certificate_extension(parent_id_extension_type(), cert_control::get_id(user_cert).str()));

	//local_user_options.extensions.push_back(
	//	certificate_extension(parent_user_id_extension_type(), cert_control::get_user_id(user_cert).str()));

	asymmetric_public_key cert_pkey(private_key);
	return certificate::create_new(cert_pkey, private_key, local_user_options);
}

//vds::guid vds::cert_control::get_id(const vds::certificate &cert) {
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(id_extension_type())).value);
//}
//
//vds::guid vds::cert_control::get_user_id(const vds::certificate &cert) {
//  return vds::guid::parse(cert.get_extension(cert.extension_by_NID(user_id_extension_type())).value);
//}
//
//vds::guid vds::cert_control::get_parent_id(const vds::certificate &cert) {
//  auto parent = cert.get_extension(cert.extension_by_NID(parent_id_extension_type())).value;
//  if(parent.empty()){
//    return vds::guid();
//  }
//  else {
//    return vds::guid::parse(parent);
//  }
//}
//
//vds::guid vds::cert_control::get_parent_user_id(const vds::certificate &cert) {
//	auto parent = cert.get_extension(cert.extension_by_NID(parent_user_id_extension_type())).value;
//	if (parent.empty()) {
//		return vds::guid();
//	}
//	else {
//		return vds::guid::parse(parent);
//	}
//}
//
void vds::cert_control::genereate_all(const std::string& root_login, const std::string& root_password) {

  const auto root_private_key = asymmetric_private_key::generate(asymmetric_crypto::rsa4096());
  const auto root_private_key_str = base64::from_bytes(root_private_key.der(root_password));
  vds_assert(sizeof(root_private_key_) > root_private_key_str.length());
  strcpy(root_private_key_, root_private_key_str.c_str());

  const auto root_user_cert = _cert_control::create_root(
    root_login,
    root_private_key);
  const auto root_user_cert_str = base64::from_bytes(root_user_cert.der());
  vds_assert(sizeof(root_certificate_) > root_user_cert_str.length());
  strcpy(root_certificate_, root_user_cert_str.c_str());

}
