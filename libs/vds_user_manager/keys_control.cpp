#include "stdafx.h"
#include "private/cert_control_p.h"
#include "keys_control.h"

char vds::keys_control::common_news_channel_id_[65] =
"WfHaf5lPHpPCJXqHahqBUZ5gwGy5B8n7Uq9BKoRJtLE=";
char vds::keys_control::common_news_read_public_key_[4097] =
"MIICCgKCAgEAwvXI1kfLosg6mUobta2jQ5TMUumETBxdzcumbX6H5hWTLK0+SfK16JvtQoAFROtundo3"
"XjZmJXKj85VBL8/+MpUE0dUoOibi9EuQ8/mNcFJArss24QmrhIUphBlwYaSbXwbzvbKvxRlh/2v/kzrz"
"hEO+to0IgwMWCs2KdfSNlCpv3gLFZfGRdRLO78onKjzUcxwrjSPZrMue38w2gOY2o2MgUFYUIwh2oA7I"
"l1cI4xg8y6IDr2K6EAzleEB0MvSJxDygjXovLJjPw0PxhQJ5h3C75SRx9CV+AqucnrXfhzJP2treqOCx"
"93AAMri2N488BnoYf3AytLY0ZnvxnC2U1cF5CF6QBwy6QjJSMfNeMawfXbOAnWaX01pucsTqimdPKrOS"
"8GT6iNyPq/mcWpAZYwG1CoI2xKy30UBJFjDNlAV4vHkx1A9dIj8DAmBrbt6S6/fc+mwi2H7jeSyj8Vko"
"2BzY1KF/1ex84xP+9NV66SC3dkyuwRWN2ToBL3lKlq2XqmgkR27I+YNowpmJd/3F2UJjBDPYeKKPOMIy"
"PNTrm9vpHP5eE6Z+FPBJ2Ge/7ntadfTfiCFC7k6kASDrh3bw8CXzg0mk08kHnJWjjcEOh3Sd6NqFXFPz"
"Qi44QMaegiTOjnwh0fwCMHFUGuw6amQwLkHquLEEkvfKeXLKyH8nQNECAwEAAQ==";
char vds::keys_control::common_news_read_private_key_[3137] =
"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"
"/88bGOgPdg==";
char vds::keys_control::common_news_write_public_key_[4097] =
"MIICCgKCAgEAy0txLz0mQpXroZ1zq8vI848RQYxq0P3H7ieKrKl5VYXzuz0Ewx2x8WFmNoyEuhlNyx7z"
"Amm9G2blTldh9NRKafff7UYicRsyj2AwbqWOg3saTLh8ymadc0g3gb56QQE1Q1CrLscxRbAHC1p9fJbE"
"5OIeV6o7Y2UIIuBhaj7wS13NzGgCMLrYT1PmB2J3ujk+yLyRxy9Rs8gDBcNF2h2bXzAZOlKRTsuzXfT3"
"xjl5/ANPa7iAbJVnha9DEy9yEv0DQTA97lL/ZHPi78GQ8rjdChgOXLcjlAvv/3iFlJDCxs+4BhhJT6xx"
"GZHeEmvHiMvI9deDk2KPTRDidT23Z3bMSeHeHWsLFmz1T1QQN6cIDFGiXlw6mwnoDX2DtJdDBfgy4EsE"
"bdizZYfJkqo7ABbfmg/da7HkGTAUprn12XmpBg7GswEnrtfYIEm3PIwg7ofE5R/Mrahr2ZdOoymnRQOM"
"jjA9+3mjunpdVqRCklH77EtiggDJB2JQyt3cHTRP9pelYBjapXG7syxW9RUFmy9dCsEh3qhjvPkZezQh"
"Q3lTkyYlSKq2yRWpJEd5MkShDal4/zvARfc534REjPHnkwNJy4uD9IJXEfp4uegTubuFgu1bU39WBxJz"
"ycQ494qvRXcDAbkx8rPog/mbsTPlnkytuJDVXxzwL1AQcOnm8SQk3NUCAwEAAQ==";
char vds::keys_control::common_news_admin_public_key_[4097] =
"MIICCgKCAgEAxrK+82uOgCGeZWTNzGhd6hNLibTboE63myFdBMl+9QDea0FsyfWKcGqRYow57EIjcesN"
"Osejv+opXFhxY9bCoqT6vdIPkNoGdo7QKYr9R049En/73LlutKOk23Fk8VM+3QaocJsfEQEpO9PPFCgw"
"vEa5h55EvgN1aVtRrInzBq19X/3O+K+tA8l/MVClxVcvI/rKnt2SUjrd3NES9IGVPA9tE1sEvuob/nfF"
"FtjsK+yJ1PXxNJK1+Smqz9W5vyzS5p02hZrUaqjxalAFpZLZL4pWzohCU1LwUTBHoYSN6BN7eHS2olSK"
"wNh7R2LU/prNdjidoEgHKBilVVRM7pfhsxmd89BH1uzWru5nHH5zY0zuVVLRzTqNM1k7Rglb3ZPH1154"
"LT/yVVMgqZzTzoP5axf8kJNseKASHMmUyQv5mqN5xlAc07YdnapEDSfpgde+7mFGGtU634PkHdVo1XFE"
"Tmjou8soLHtRMYXTMi2vvlxDSsjO5vE+45d7Jljh80p+7b1296hcJePcVWWINruA2uddjeddl+6wmWlT"
"CL4Gw7nnkILnkoVo7RwmgvdkcXCkZdiHY4r5WsYudLsGI72fv6CEOSJ+gUVigMxO5x8N+c41DxKiClPm"
"1QmN/kkBLjppzBwmfuhE0YV35APe4HrzeJ+lqOEPmt9JOOgtq5753WkCAwEAAQ==";
char vds::keys_control::autoupdate_channel_id_[65] =
"RGEN/QEoSRdMfFzRve18LLn3llHwYpM7vvZJ/gz+W5Q=";
char vds::keys_control::autoupdate_read_public_key_[4097] =
"MIICCgKCAgEAtNFmgbQDI7gqWUSSPBUjEe+e+9Q6NTSpLjxKpNEuY2VJ2b8H1uBjwae98E/Vp3TGIZVj"
"N+VPDZYoZbvuHOxUZBWWv/cibBkF65yH5U+qYcCkVhoDYo+jUiJ4UlKBpUUkZMRIZeYtqm8BWvu6rljq"
"3LIisRw6rpEsTsMyT3OxmKbrETOKcKkll9DqyEWFZ35AQ2caJ/VplO/wm1a+ddX7zGs/4ghMR/Yq7FZi"
"BZyZ3sG+gbyOxEnnBSrHKxzJBka++KAticM6ZHPVKEEmXOPOCu1Y9jJGUZvR1fMwl3xyWGUHlKOXb1ZN"
"BIpp+cGrwsCpG/4QQyJNpK7r7TyK/4w/ukiIsNqAx7Y0TK0MlQ8pgmt2GrK0HEEN6jo3oCETH7YXnCjk"
"hU3HVLCh5Kq17EK3fA/Q07aY575/L8hWpHSWxeE8hib1VIM+M7PHexQ67g2gB2h1dqNmYKzmz5u9fqk7"
"a/qcv4sScpkGQldioFGo7KB+FtJttUJ2T8WvLgjhcyFUidYG1uMwMfEMzz5TU0UKAnY4ARmqQbXqXgQn"
"d0s7014ntFymwnKaymkl0Nq3hjCbfva8WT7vUqFYWvJ9c85MQT0wLQyOPZtbEwqdUx36BcSkRtsKhJEI"
"mIuhy+doM6tHrcPI3C2N81VY/HpQOICNaHPCDsAksJ+OXZztcxUaFSECAwEAAQ==";
char vds::keys_control::autoupdate_read_private_key_[3137] =
"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"
"JjEd9N5hFAg=";
char vds::keys_control::autoupdate_write_public_key_[4097] =
"MIICCgKCAgEA1SXyZW5lNT8w2w0hfIVmcGQjXt2o3Gh7KflJ7JOiZq6e6ZaljeemL5KKFGhq0o9l0G0+"
"BMyMkJkWgW2BBNXtOzShWF9DC2Zxbeq0l2jKBrjW3TJz8cTQRujHkjmtI/qLpVMLDGksavpTWe7ORGS/"
"7UdZNPkilVcFy0EEplb/asP3c423nWFW78G36aQaorZ2atlZj41ouCYt2QyeZbWBfkF+tN44vi3xHfkH"
"YLat7GLGSHvRIx+Conf3nDgo2tcBMU7HgM9MrjWcBfbFM1XVcLPgHqyRswWTKK2DGmE+NB1iI7ISn8DJ"
"cxEY1KipV2vhP0zoDTz5tICEQ/3lPgSyvufWzOFMhQj4ShbXznCppem2CXDr0Z8OT8pRBF2vudSbTFvZ"
"kxbpXN22wAnJic4vPoMPIn+cK1Q6iJseSHSDKvDRecKj949mWldpnN816dEVeTWn1rzqvmeZplR/A3lF"
"UVdxJDKrCOxz7zZuLyLP15pafDlF1DIMIvE4Ey/fzzMeP0oWW5y3kbi2IWucrXvywQ+CVhmMdJWezfL7"
"u8BoIMoWlV5bkjwO1ZC1W1uyRavXuW6eoJhmAuFQE5nkJELyOiRB494ndJAsbTfcEs2qVdDHMxlsE1ev"
"e0PBTRiaWynWUsZ0wgOsrU11KbKAPMy7Gj4fctAc1F3tuK70qLaIXzsCAwEAAQ==";
char vds::keys_control::autoupdate_admin_public_key_[4097] =
"MIICCgKCAgEA48KFlP08CfVz9JYzSjHHg7LB3EHfe3BusMJ2fQcVbmqQ0OYpOzmtTr4aFGwHFxWqNefN"
"b99yp5h+r4nFtBWiROxAypXtRx9v98FQdCn34zLJvh26EVks70o8F+hdKF/tb/MZepfg9wgjWcFIIgSe"
"/KKoA6kwBaSc3QqSuQflHXLp3KlEDrlmpOcBXUpM6CQYWoZ789IL5F5ZTxXeilXNsRqLs+aywe0ZUenx"
"yj9sd1pxrdcpkpMmCmL7H7nYzM4wc8Rl5meB0DFOCeJVTGTr6GO4wwQY3lxtFUH2dKima0WoMKdS1o0d"
"SgGVrR/MMN0t09VUysfqt4lnY7jd40n2YePdSbzCMWbtXa1rsd4thmtvErLOym0VFB4eOQWVQDHypOj3"
"BuGGor8U6Wj+g2xFXSetamNSj4VlMqx4KIqxer3mSxbLSWGlZ9thIxTsKuPBbgDkKeL0UYie0nSaUh9g"
"UVaLxq0D5yt/T2SU7MkGwBogE1Y+uXtRP78V5Gt3fIz0YHnRPd1AF2wY8Fi6fiaFJt+8/JSIUP/qaR5d"
"8/ngT1bjE30mXhRo/Yd/RZQQ8bExBNM7oAWFR8HkLbuP6uHPtuBn6ps4e13rQ5DJ4Btg6j++PFNX2GRs"
"kxBbgBlVqeSSkDsdFG1hRxubwREcfF3FkCmXIy1t9QQQhhznRrmIf4cCAwEAAQ==";
char vds::keys_control::web_channel_id_[65] =
"yXYa7P9ebWf2TCmeIWYQPQ+LZS8Lc6VOqzfzCSTPLjk=";
char vds::keys_control::web_read_public_key_[4097] =
"MIICCgKCAgEAu94rJGuDJHLWij1/03Bz04ja36FufAUDRUObOjJMcocEt02UWqKTqUIZrlVtnNnnYKbO"
"KqywTw4fP3mjJL3v8MeXjYCBGDOOQusUL2PFSFBg4jccLtRY2/Omg30jpPv2CR5nHcuMn4EmKzk4rgFw"
"rfBKsbQY9CWkbYCgc9o+dOMzNIiToZ97MtqGZ+QcVEi2rhBgFMHe/39G3dkbjluZy5SLXXGIXD8mkw0H"
"sW64gvBcm/Enc1XOHfQLSPPL7PiZm5La72K6YN7gEkh9etjmXn2FAYPJdb2bI8YRsmdxbwGUT8J3uUsp"
"Qi7StZqgA848iVFhnNXcCCrsyh0+tUDOdNoGUc7AajP1VolG6O1vJPNGKRU6LzXaWLpirjcD7SSuhEQG"
"bhhgcExQ6woOmJQIh+XBYFskKNZBnvht37ne0UNMItd4qWlF4xHIW3WyPXAl4U5DEsmT7dq01Gl8jLg8"
"NwQV6ahMr4rhLX+coV4xCvMyNTgdTtA04z0nK2wMJcj89rPbPvwx3iPtMNP2/eEsj/5IFiZA7IcDjE8M"
"11Y5v8MI9YwSxIuvfIOTb5RNL/ghTP55Yi7qJaOQ1sfA9xxX52I0VQJwC/bJuzjhmFWINxZxtYjhCoX2"
"ch7unRb2i3WV5uZyX4qsH3SjKTUnMestxNXlU4Zp9OwBXBQszobRJH8CAwEAAQ==";
char vds::keys_control::web_read_private_key_[3137] =
"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"
"IeKFyb9Q5hU=";
char vds::keys_control::web_write_public_key_[4097] =
"MIICCgKCAgEAuucjCwt6KQLPNgEBQPiRxQP6TGOBkr48y9RxMLfaFUpWRv+29hITwlmcvxYwKtmVaiZk"
"CHVw2Uq3ikw0WeHuqioTSRBlkVN1ngO8Y/gu7w+Q4B9HfMq9bMlDotU4PNEvn8BZlfVs3NGE6RgPolWN"
"6Go8DD9HNpD/u1U97ISdDBkxrtCmfCLnDGKs31b7bYSRyKaivnFsLtwt1R0gw9N6H7Jby2YHMCt0ThKJ"
"vWl7omQwceG3YTEWa0bKCN6hEVWckMeuGOVb5exjUqRTgE+keukveBPPzp1ExLdpChQH8RvLOdUgW2X1"
"BD8RUQ59iw9EYHb4kQ0kulHRj1Se+VovUZ7RNWm0fBI0o/aaQZNz5Dw0vrEH10mIA8vYqbfpQEiJ64s5"
"GXzHKVdpTHZnpfGWhH/eBGWvMWUtrI+81ji6oo1ziJ07O2ytaKch8gB54Tmbs3tTsc77dtLVNbVr4wW2"
"AyLr2opUFE7XxjUznLX90wKgcEL148q07AzuJJnLaFOc49c8dOOAKdRyESio6zWevt2NfzQkeYgJ7PrW"
"k56kQGqOJBQgNJgyDpHZqA570ob9x1MLXCCEVQ3AI5chRtI1a9ySfOxig4kzxwVxY6DdJWt1s6EDhqyZ"
"/mRvieXspRPI63vMXSDvBStuTScKFNO9x+MMglqkYldJYMrTdrzLGscCAwEAAQ==";
char vds::keys_control::web_admin_public_key_[4097] =
"MIICCgKCAgEAs/w9yWkaNQU2s3BVmY3M07X9MDqfOuf8LdSN9an5GZLbLFmWL8dBrLG90+DOHY0T1raL"
"ZOIupIHWp0FUhIb00erryKb0mkEqJ2IkPBvnMZpI/J5yTOZLaTzbVJK4N+epcOlQ5A62WAnP0fb1uJOi"
"Pe8MOLvSe96/uyBpQZrcgAzUi0x+RJab/rersHktKq/MfRPBJFpAgej+cu0Ai2W6PS1zbTOn6Ixfz8kL"
"5+fuJIfZgdC5P36jqNVzsS/KlRVIU+nCyi/uEOMY7QpkOespCX01NCGNTja2QMtbnmezMfCcpXeAoJ/Y"
"3iDo8o/gPKAdK4oDOb3kcJ/G05qYio122qqkqljdsvsEdZH59Jtsz3/i4muHX+RbNcp5y9yt4OU/QE+j"
"tbY1xJ/Mh3mT1a8ViSyak58NUUQ1geEG4HGa4JPX/UN8iu9lOm66NIUujiaKyQLYfWDKh5I/ghyQTKii"
"+gOQyhZRqgwRc7/YTwFEb92jjwCDXAFz6DqUmQLkfjUVm/qs8HIeEuRvOW8ud65WrXKu28JKAKKQDgd8"
"a/Zbc2J30o3UMQMFQZxVpjoQrUSubqmHSuTtvDZBhTKRh/5PNzbAy8adnOMyG03kFw0VhGu+n6OR7uOD"
"RiRM2J68W/VrrsygVMMWgmO7nSImA7mCuGixaNNiiLmhdKwUxdrjT/0CAwEAAQ==";
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

vds::expected<vds::certificate> vds::_cert_control::create_cert(
  const vds::asymmetric_private_key & private_key) {
  
  GET_EXPECTED(cert_pkey, asymmetric_public_key::create(private_key));
  GET_EXPECTED(name, cert_pkey.hash(hash::sha1()));

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
  GET_EXPECTED(name, cert_pkey.hash(hash::sha1()));
  
  certificate::create_options local_user_options;
  local_user_options.country = "RU";
  local_user_options.organization = "IVySoft";
  local_user_options.name = base64::from_bytes(name);
  local_user_options.ca_certificate = &user_cert;
  local_user_options.ca_certificate_private_key = &user_private_key;

  return certificate::create_new(cert_pkey, private_key, local_user_options);
}

const std::string& vds::keys_control::auto_update_login() {
  static std::string auto_update_login_("auto_update_login");
  return auto_update_login_;
}

const std::string& vds::keys_control::auto_update_password() {
  static std::string auto_update_password_("auto_update_password");
  return auto_update_password_;
}

const std::string& vds::keys_control::web_login() {
  static std::string web_login_("web_login");
  return web_login_;
}

const std::string& vds::keys_control::web_password() {
  static std::string web_password_("web_password");
  return web_password_;
}

vds::expected<void> vds::keys_control::private_info_t::genereate_all() {
  GET_EXPECTED(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_write_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->common_news_admin_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->autoupdate_write_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->autoupdate_admin_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->web_write_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  this->web_admin_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));

  return expected<void>();
}

static void save_buffer(char(&buffer_storage)[65], const vds::const_data_buffer & data) {
  auto storage_str = vds::base64::from_bytes(data);
  vds_assert(sizeof(buffer_storage) > storage_str.length());
  strcpy(buffer_storage, storage_str.c_str());
}

static void save_public_key(char (&public_key_storage)[vds::asymmetric_public_key::base64_size + 1], const vds::asymmetric_public_key & public_key) {
  auto der = public_key.der();
  if(der.has_error()) {
    throw std::runtime_error(der.error()->what());
  }

  auto cert_storage_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(public_key_storage) > cert_storage_str.length());
  strcpy(public_key_storage, cert_storage_str.c_str());
}

static void save_private_key(char (&private_key_storage)[vds::asymmetric_private_key::base64_size + 1], const vds::asymmetric_private_key & private_key) {
  auto der = private_key.der(std::string());
  if (der.has_error()) {
    throw std::runtime_error(der.error()->what());
  }

  const auto private_key_str = vds::base64::from_bytes(der.value());
  vds_assert(sizeof(private_key_storage) > private_key_str.length());
  strcpy(private_key_storage, private_key_str.c_str());
}

vds::expected<void> vds::keys_control::genereate_all(
  const private_info_t & private_info) {

  //
  GET_EXPECTED(common_news_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  save_private_key(common_news_read_private_key_, common_news_read_private_key);
  GET_EXPECTED(common_news_read_certificate, asymmetric_public_key::create(common_news_read_private_key));
  save_public_key(common_news_read_public_key_, common_news_read_certificate);

  GET_EXPECTED(common_news_write_certificate, asymmetric_public_key::create(*private_info.common_news_write_private_key_));
  save_public_key(common_news_write_public_key_, common_news_write_certificate);

  GET_EXPECTED(common_news_admin_certificate, asymmetric_public_key::create(*private_info.common_news_admin_private_key_));
  save_public_key(common_news_admin_public_key_, common_news_admin_certificate);
  GET_EXPECTED(common_news_channel_id, common_news_admin_certificate.hash(hash::sha256()));
  save_buffer(common_news_channel_id_, common_news_channel_id);

  //Auto update
  GET_EXPECTED(autoupdate_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  save_private_key(autoupdate_read_private_key_, autoupdate_read_private_key);
  GET_EXPECTED(autoupdate_read_certificate, asymmetric_public_key::create(autoupdate_read_private_key));
  save_public_key(autoupdate_read_public_key_, autoupdate_read_certificate);

  GET_EXPECTED(autoupdate_write_certificate, asymmetric_public_key::create(*private_info.autoupdate_write_private_key_));
  save_public_key(autoupdate_write_public_key_, autoupdate_write_certificate);

  GET_EXPECTED(autoupdate_admin_certificate, asymmetric_public_key::create(*private_info.autoupdate_admin_private_key_));
  save_public_key(autoupdate_admin_public_key_, autoupdate_admin_certificate);

  GET_EXPECTED(autoupdate_channel_id, autoupdate_admin_certificate.hash(hash::sha256()));
  save_buffer(autoupdate_channel_id_, autoupdate_channel_id);

  //Web
  GET_EXPECTED(web_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  save_private_key(web_read_private_key_, web_read_private_key);
  GET_EXPECTED(web_read_certificate, asymmetric_public_key::create(
    web_read_private_key));
  save_public_key(web_read_public_key_, web_read_certificate);

  GET_EXPECTED(web_write_certificate, asymmetric_public_key::create(
    *private_info.web_write_private_key_));
  save_public_key(web_write_public_key_, web_write_certificate);

  GET_EXPECTED(web_admin_certificate, asymmetric_public_key::create(
    *private_info.web_admin_private_key_));
  save_public_key(web_admin_public_key_, web_admin_certificate);

  GET_EXPECTED(web_channel_id, web_admin_certificate.hash(hash::sha256()));
  save_buffer(web_channel_id_, web_channel_id);

  return expected<void>();
}
