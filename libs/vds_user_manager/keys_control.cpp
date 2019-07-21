#include "stdafx.h"
#include "private/cert_control_p.h"
#include "keys_control.h"

char vds::keys_control::root_id_[65] =
"oc9RuWeOndq1U64uID1IiJSTX7dc9oE3DmT3wH7Gv0E=";
char vds::keys_control::common_news_channel_id_[65] =
"vD2BQn/Vq6jWuWag5ERFsMyQ7V9CNaMMLq0xhNfT3iU=";
char vds::keys_control::common_news_read_public_key_[4097] =
"MIICCgKCAgEA1WjBXb2FZCVfLBamCzHpyxgDiHkRvAUSBzbMruHt/dA8AQqCM2q+LzZT7zWD6R/XQRMo"
"Fyh2A9PL+vkNxA/3DUqgJLSqYw2NSY615+54RJv/N7Oxb5WgT5yNC2S4Zedl8zmX/N3lpWEiGPt8lXjr"
"m8NpNgWd2VNcSPwijkf9X/MYmen3p6/9E21LTR1IdpucoCoiu2uwrVGDLkl0/Xx6VdCL8L8xBt+ayA/G"
"0Q0bAQTBu1+hheYNgU1C+TXhs6Fe1z/CMWgyefpJ0JqFBQuGeiZ5g459aupGniKFkHsJEBopVP6FJ+Pl"
"eTPB2OOO0LCvRvRsruMYGepoP84z8EKoXmnwN7I6LOPHuB2dijSSzv05hpfeuRPH0C3tlSRmChugzSZy"
"afNURIoDQaqnwnHwNNXN7OjK86qwJW03vILpy5etiKKiBNKnjE3oJhJ7e3SEmnIEWdXI9sY1KxPCqJ46"
"L3H6YQBUVj/pkvm0ELL2rk0X2A8fZBJ8dDQeo9unI0wHyOMj6pd0l/ujCf6rzvIZfsDB+zd3DsdlrwBs"
"I3pH8nY4YKo9TwWVhQpeRnK7P/MGLV76+AhCaUbSP6Jy2ji22+ZNTS+AiD+KqbGG/3WCC7IPW13prS5X"
"mF4FRz+gt/gyPiIudy5YwmfBgX52r0YGPp0zpQ3+FX75shWHhfbsBZcCAwEAAQ==";
char vds::keys_control::common_news_read_private_key_[3137] =
"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"
"WfA2/yAOQVs=";
char vds::keys_control::common_news_write_public_key_[4097] =
"MIICCgKCAgEAuqbsgmd/TaPO5Ija9iR4qnIfAx7/W/GbTmPiRxJAM0okJf37CwFoRHaqcFoWXTKa7zS6"
"iGfcrYLeaiNOvqbUrWlZ3+MQR9HJlpPSjkuwZbpKvEFd1j0dFd4RMi8Li+BuQ8JLqJFMu0LmSewx7UJT"
"0G64+GrjPlc2CjKNZ+MTftbCTOrDguOEqHu9DEj4BQOmZRSlTmFs8LXQ/ZfVf8jrEOBAoQI9lSviMefh"
"XwkkvR1OORW9UwHIe4uE2x4BLirqJ8glOvcQI1o6uBAsDsjYK7p7MJfYw1pNR71QBTz8dcrXiB9/n6u/"
"TyrW1350mcFhO4GBWTljcOZFo+k4Jj+D2/8AGjFmWqHj4t/2b5lvljCLCj9JkPKh8ec691A2rRlXySB/"
"WIQGQUOdqcFRGRUxRljCa9lbaws9z7EGabfrB8mExZzxkCRN805qhs0V2NzCEmdVN5xnM3Ed4qciucXi"
"pyLtjwfwYzVxVaXzQ9WUBamqNPfFQSH319L8OwWUurcGVgHW+OlUkMt6TAuOfx/rcGtpoYbVBqahKW6B"
"4FjE6EuXl7rXe3utxv3EnaGFq0qSi3b9Bi0TpCwNAmEyFvgbIDLTpuDI8SCDlzHcMVCyRMT0oU6WouOR"
"VY6EHrSx0+IM+BI7dwK40p4PIQtjU6JVKhDbiT8F9jHlMaYAC+0PjykCAwEAAQ==";
char vds::keys_control::common_news_admin_public_key_[4097] =
"MIICCgKCAgEAv5K3n7nQI0Xkw/ah4Xssj/14WskpgKUHgLOgVGFIuDKGYUawlqn7rXfa9ruGu7oUPk/h"
"yHW0wN5Q7GPrwM84ogU20KPoVuZxOXxqF3KRGBykSwE1jyZuLJK5phdITECzHHuuXxBguwwBBuOulmAx"
"7zzSk02tFf7OcrdbLK5GZAJkVqwKOKRAzdBJhbf2J1jG6hg0s4+lt6076cRBlNwPpKyKayFIQsLvS5Uu"
"IBDetX2X+ugpI0yhjcIBlXB0ubM/Y0CK7TgcWFiMRejgFkENOesbpIySFroLyTMOLed9pUB91aH4ycQZ"
"pYPyeH5LfnHFCNSOPUyPk/nnwDA+rn4igB9RDquV1hsms31geyfAYZv5HNO24IGiDggiYTKJVM4SQuyI"
"OXX4uRym6B8f8lynTPSdomaDmJM6YbBRHinRCVLwWcM8SQ2EaZ0341Xr8lTvP58qFXRnDLrYN/ph61jF"
"qZm9YR1uvxiY0ErjHG+ZaQ4RPZukws+NxSGzgDd0EkrpppN4uPm2X1NtQ42fcdvAjg7AfkuQzRHFOqdu"
"wvtK/jWEsOmyx/f901stoHzcXG5zHSFvb1amdveYqDeSRu6FLEPNguV9aeVdBc96aUyas/eMTSrZ+mVq"
"LREbIchyn8yfJzSK4u7pLEaTc0DTb7pciSW0zxdce9JQnrfPfwBfdG8CAwEAAQ==";
char vds::keys_control::autoupdate_channel_id_[65] =
"Cs5OGyzvBn0J5/jnW6PdhxQsz/elEOtmuEP+7vMsJ1U=";
char vds::keys_control::autoupdate_read_public_key_[4097] =
"MIICCgKCAgEAsAGSTNMDiP2naP6PzHHYz8TrdmzMec8AYGXCdHTNuVdisHtqde6U9KLXCnOp4aDMEPgq"
"K0/C029qgQjmwGZIf3BUrJfVDuxBReMcuL5u9qWI4SqLdzEY4tBuYdLnbTBYD/arzGOl2aJQL/uceARv"
"6wZc40Ccbq+JRbtIm5haYRiFJ7rxPnczk67TZDGNGI9ZLZzrV+9UKTitT06RdQQnDZntkNuXwKwPQCK+"
"FbgEDIug7oIN6HDqd1krnl+kT4pAc5zIS4a4pdMdxkMtkq6+DwE6EsImLHB72c1i2VvXcVGd6sSL8A/y"
"BSPyT2840Gz+lIniPacMyVUzFCa6iO33/Sx9+mxzdb+oW5WTerhTM9NuxU/x0sZFj0mQJWy/fqunAmXf"
"OjiCSG8OCa0jahP6jEnQxocwpUTQPaXX9XNMOByDLMmeP1woPFFISxea8m+e8PEdG5edxY0dfoX5UXIw"
"M/HPRA/dC5q+NSCfCsLSUcPQURpl5N+x3d8H8P3TumAVikklQMlvM7PE+fSxovy2jThKbhhw1QyVY+9G"
"Rf7oK/0Ixw944NsIJZ9a7CgX/mJzZ18xRcYQauA3my+ZEr3KW9/APgcEKMIbO9loMG8CL9m4XCNufRgC"
"xX9brLfqQisCnuDuiMpHiAsZBxZTodFBkbTrFMZv8l6SXlC0EnGoBoMCAwEAAQ==";
char vds::keys_control::autoupdate_read_private_key_[3137] =
"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"
"XCC+0ZeeThMd";
char vds::keys_control::autoupdate_write_public_key_[4097] =
"MIICCgKCAgEA012ZKwk5a8/Te2FwC/QJ4zPOR1Xl5S0Qn3Z9ENZeCZ86athGIB7s8tKFUbzFseRtRs+F"
"UTTIiRiHFo3DrEzri9QseK5lXNaDnaVibdXcwjnG6VgQkcidxswzlAQPdBQKzpcI+V8G7gVw3aUQAWhr"
"SvlD8VoY/ccxxi4oAp2vCqqdw8/ySE6OGbmCik/oYdJeAptDOULA8G1b49ErTSrin/W50jxDjBL4+ibu"
"d39cQCziVNfwGYYdUUfg9Xdx5epQ5Zx6pX2Gs8RRAgVJQkMlCFV1bpRheS27MJV31y6ExWqslu9PItrw"
"8tdRlaYSIKvh/S0/5bM/KDt/zHC1wKd8atacILMGfvYNu1rM95cxlV/jBsN8Cncze0EKmlvzDdUbfXvC"
"/65fOea/PdQ54RFhyWhXgpNQ+wc7t6qP1kp4eAtcsViUGSPtc74uAa8fIvCSzzk1OKzX9lrs+qEWNqWq"
"usHngzBt9sccKBNZxPBoNhjKIZgP6sBjmTHWdKS14bSmYdm2fGlNsWDwA7AgWEfuTYT0ZsKzvWlMASkA"
"mU1ZVdzxvFYxOb0H6DLIlw2/rn4I31J7Iahg499WP468/i2rrhwB/puEXY/++udYN9puCycWMj1DWOXe"
"28ZEffqtX/t2M5OFivMoY+LRKjXaivq67/ZTYHdhGg1FSTuUOhhFbwUCAwEAAQ==";
char vds::keys_control::autoupdate_admin_public_key_[4097] =
"MIICCgKCAgEA4ICbNJ4kgNCq/F731eNo9xlFQ+ZYO3z7z/28HFo+Li+img8wfTfXgefCZsXlpiME3GIk"
"oqnTZdpVAI0oiWkuTmqRKmuy9qQg3nybFXIEzlnT3cXJ8QZIcbzS/EJZ3RdCelNwGiBXAOWbxKg5uu42"
"Qta+rn8iv6IslcZPzfpxJqsL47klPhOM5G85fmosoBFfsu96rBH4vChp7E0ZVB+jOJU1y7N5U53jKFtE"
"ujmvpUGrl0sAkdd94andYjRQZ12bdQ2GSzsM2eiTC32eRusLfg0vMEo7p+lx9vXSIYQ/FBnabAQSvUKj"
"4TWLgPAqn8A0wIAXQvXC/dfua17RTfo4byK7zhhk7a1R1uC/37DlIBggbvfSZeKHCUlHXrU6j46Uix4+"
"096C40/70sZLf09tTizPqzDHWUxMOXak6HVAccVKuMdJY2GOla966pMbCB7Og7HOq7zfAwnZ4vryCn66"
"foR8t3E7VPXU83GkUZaQZ/Z3Y4snFnyNwxxGfKEh7TXu2T3HkUyTMBcuAUhqFwvBtzDmm7ZT2vL3GlWm"
"J3qrY+qtheisHejgiGn8nOs9ZYZSDtG1Of1dzGOX5GHvtGhBTdwOGn/QgT9Nk8kQFuybOKgWgBmKDHvU"
"PBc3KLTL4PjkkaCOAt/2GnLuCWIadOxHh7oz4GwJVf4n02VRxbqKl7cCAwEAAQ==";
char vds::keys_control::web_channel_id_[65] =
"jRm9zhS9uChMSeVivjStJy/BQrD4XjdvI/sRNBidRd8=";
char vds::keys_control::web_read_public_key_[4097] =
"MIICCgKCAgEArUNq0az9gOUOW7wwEHEWv12wuQ3voPqfCL94WBjkEqOOq6hE1uLgv4OyvgVF9D8WJFKo"
"hszSGxWNBeaXKPxK7FVO5g8PjqvZ8Liy+c7MIix+blIrRKNQFSTj2IAIvY+zghkjttfp5UHcTe1kjgES"
"2SLhoOiwuvYf0y+Jln/O1Dj41kUXJhS9MD83P/GG27VD+7uDbo6OrAEl1ftsC4OvPOMxLHwZv+e4QoS7"
"PZ8HBF6JZYF+L2AUv8NUpMzrroyj3OMaWyjcRuOtoL3QzU4iOGdmwoPV/5NbhNF6ewrXRIhIXjdk7gSL"
"PT0g8vCZV56JqqfJQTQ3bikL4T7GlHXkKjVMr2Q2f1cirL2TPOS3MfB1b3160mRMywYWsJQM0RId3vLw"
"8dGSYlYlTs5nzb8a9CM31/UToWx0YRXzTdxYbFvFfa1vuRqutCd9qNEYerU6agCZRtq9d8EFz7eB9Owb"
"qp0M3zJqHxlTIxgwYS9Mf0+cikmq8wnaoWiTgVuOGAMtr9HcN2+gf8bH4Xp9ar+BBIxhU5BIB5XF5NKK"
"T3DGXbSY9//ebndGfq6aE+KV1MIohZJ21LPPyscTByMkMV86Fyg1SC4z9AwkvDlsIqczldKx4WugWgdI"
"SHkJ9ezkMyIP5lllUdwQAB9UDMJeXu5mn2E+3phJCw84AO4OcRiBqckCAwEAAQ==";
char vds::keys_control::web_read_private_key_[3137] =
"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"
"vjetCS5QfkUP3a8=";
char vds::keys_control::web_write_public_key_[4097] =
"MIICCgKCAgEAwyIWJOv6hfP30kvm94MSW+pVloh67HtiMP4Me0GJplo8XUE350UKPV4YPPA0vVSkLn4D"
"pkJZvuP9sWHUTBJ5n9mSLMuDgHIGojmTj3c/i6vavneL63VrwYOAPCuIENWq2sDTLJM/bb33/takfpUA"
"NckoidQgE1m96ub5xiUSqob+Dw2V5K3Kj/duV58+X26SckmkgspDY1qfbzmkf1Ds87HGEXP+bgAo6W84"
"N/zNyGNrttsb03fz6l18TuyMn5BkxLUILMc9MyfbgX0X9Aml/2CcND3YMAX0TqVp20IRIDnOaMCuZsor"
"WUUDKzWiq8XpRh5PRyVmZeBytXXcdi1BEgnQ0sVY3Zh4XUs3YnhDfqQP05C6NJo8ZYl/F1DZ59McKmVa"
"saMMUh9JVUatfzZPeKuL44Iw5JiR3+9aveBAsYxBuLINAk16IB0FFZD2VvzzaDsSPZ3JOuOto7UNasH6"
"2su7SL1ACbegD7oLOl4rQg4XEZwaVrOIcs5491NaNqKaQVWSxQhj7VENtmWOmYz+SmptukI4y5PKcrrp"
"xl/9jxs1KOBly2Mx2twLKDk8aNTKBISBnxO4WUJCBJ64u63QBMWlaHRtlsJV2gWevLIYb7cNuUlZS2p1"
"DZSf/VPXccUSQOPjkB3MMmidfGQxZzHWlIAuwrKIfg5WBOR9O/luqqECAwEAAQ==";
char vds::keys_control::web_admin_public_key_[4097] =
"MIICCgKCAgEA1GLBjFlCE6A9s5Yw9DeZLtumInnRPh2LT9002Hy7FCMvsPXHE5jrYz3YCMz8fFDbcC3n"
"bGbvbbbzJhmQik3aoBw0e3GId+J77pWnUj2N/baQD8ATtQUJWWPjBg+Br9s+qjdvRcb3ZhEXNxY2g9pg"
"r91oTvTkXdvoDuk1IBfF7huy7VEIEw+3ppsx2XfCPIpLehoIaB3nRfBV2BXCSekU9fDtJkk+hPWbZBfO"
"uVE8uK/4AEyRNci/faD7PUB+0UT7wcolJbIF/kZXAT1sv6FHQqQGSvrkDGd5kBfGUxcJf0OJAhoAvIU1"
"3iq5DeZ3UenHqVPGSbi6SjTGaAO8BQjQ/Q6Ulppj7vuu9eCXYh+kQrOVwRlu5LaNoU/1cOeGscVeZHhJ"
"gkn/wkOjYr1t/lKqN/Yb5QXvtz59EXvlVkrY1JTqwhTrHHPEy12qIgWjZ76SFXh3pmGFe9uyTi1ylLnV"
"LAlcnPXzp7qE5ER2fMS7DDbgYpm8HSTUomlFTog9jShDkP/ljg6V66UT4p7/4bNAa4S9eZ4znbBqdlfn"
"KIztPUZ5cJJH8vY4VUC8tgztFN8XSLMv4huaWgN6lUm1oIVgM1eduNxB2tKxUTK0gLyxgOPXZ0Gr4DXi"
"kkczv/NbQwzkuQJK+fvBdxlSQZ9YCqOo0kgUi5cE3ynCcZP6xMBV3j0CAwEAAQ==";

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
  this->root_private_key_ = std::make_shared<asymmetric_private_key>(std::move(key));
  
  GET_EXPECTED_VALUE(key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
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
  GET_EXPECTED(root_id, root_certificate.hash(hash::sha256()));
  save_buffer(root_id_, root_id);

  //
  GET_EXPECTED(common_news_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  CHECK_EXPECTED(save_private_key(common_news_read_private_key_, common_news_read_private_key));
  GET_EXPECTED(common_news_read_certificate, asymmetric_public_key::create(common_news_read_private_key));
  CHECK_EXPECTED(save_public_key(common_news_read_public_key_, common_news_read_certificate));

  GET_EXPECTED(common_news_write_certificate, asymmetric_public_key::create(*private_info.common_news_write_private_key_));
  CHECK_EXPECTED(save_public_key(common_news_write_public_key_, common_news_write_certificate));

  GET_EXPECTED(common_news_admin_certificate, asymmetric_public_key::create(*private_info.common_news_admin_private_key_));
  CHECK_EXPECTED(save_public_key(common_news_admin_public_key_, common_news_admin_certificate));
  GET_EXPECTED(common_news_channel_id, common_news_admin_certificate.hash(hash::sha256()));
  save_buffer(common_news_channel_id_, common_news_channel_id);

  //Auto update
  GET_EXPECTED(autoupdate_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  CHECK_EXPECTED(save_private_key(autoupdate_read_private_key_, autoupdate_read_private_key));
  GET_EXPECTED(autoupdate_read_certificate, asymmetric_public_key::create(autoupdate_read_private_key));
  CHECK_EXPECTED(save_public_key(autoupdate_read_public_key_, autoupdate_read_certificate));

  GET_EXPECTED(autoupdate_write_certificate, asymmetric_public_key::create(*private_info.autoupdate_write_private_key_));
  CHECK_EXPECTED(save_public_key(autoupdate_write_public_key_, autoupdate_write_certificate));

  GET_EXPECTED(autoupdate_admin_certificate, asymmetric_public_key::create(*private_info.autoupdate_admin_private_key_));
  CHECK_EXPECTED(save_public_key(autoupdate_admin_public_key_, autoupdate_admin_certificate));

  GET_EXPECTED(autoupdate_channel_id, autoupdate_admin_certificate.hash(hash::sha256()));
  save_buffer(autoupdate_channel_id_, autoupdate_channel_id);

  //Web
  GET_EXPECTED(web_read_private_key, asymmetric_private_key::generate(asymmetric_crypto::rsa4096()));
  CHECK_EXPECTED(save_private_key(web_read_private_key_, web_read_private_key));
  GET_EXPECTED(web_read_certificate, asymmetric_public_key::create(
    web_read_private_key));
  CHECK_EXPECTED(save_public_key(web_read_public_key_, web_read_certificate));

  GET_EXPECTED(web_write_certificate, asymmetric_public_key::create(
    *private_info.web_write_private_key_));
  CHECK_EXPECTED(save_public_key(web_write_public_key_, web_write_certificate));

  GET_EXPECTED(web_admin_certificate, asymmetric_public_key::create(
    *private_info.web_admin_private_key_));
  CHECK_EXPECTED(save_public_key(web_admin_public_key_, web_admin_certificate));

  GET_EXPECTED(web_channel_id, web_admin_certificate.hash(hash::sha256()));
  save_buffer(web_channel_id_, web_channel_id);

  return expected<void>();
}
