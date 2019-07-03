#include "stdafx.h"
#include "private/cert_control_p.h"
#include "keys_control.h"

char vds::keys_control::root_id_[65] =
"SbYELRyYJwoT82HUevkPFDQvHQrxOKFj/xDUS3xt8uI=";
char vds::keys_control::common_news_channel_id_[65] =
"2+ucNFbpIq13nVp2KUMQ1GcaO4U3n2DBfYhNpFb6ojs=";
char vds::keys_control::common_news_read_public_key_[4097] =
"MIICCgKCAgEAsTTRizn3MwojBSN7GfN8HKYDUBs462Kcwm0T8iWGEsH/SrT57vAHqEGyIwxt6OzYuN2j"
"ku/4kYrHFI/4xUNmiZXLi8iwgyv7q3msu7t4p3Q3DB+BCCcDCXctfJCZGdUFPntGrzQWHo0NPnak+oZk"
"Nv1eK+/zRnXq+c48lgb5OGTxq6wnvy3z1TxasYlyGXV4RTXTYqpMXUNW/VIN3l98Fk02kbTBo9UI++vp"
"QgexMij5yflZ6VwSRTTuUws5nk+R3RchkUUo368/+/TeTw1MQ88cVk6YEG1XRRFcdFdM44ANiiRgm0LY"
"X2LUXZE77dJPDb39xb5EpZADF7LfkJivdZEDjwdJWypRQkrEQ3s8yV5hD0w7UZbUtIQSHy/TDk1sNT/K"
"KwIT27zWR7Q2VQBh6xzyzvYiXabAPRRvWfk4sipMvg1TFMvgtjo0ItR9D2t/klAV/XrPe9ZEc48SDm++"
"EAPkIOsZsC+r08FwJgfcocP7BSgSVRiqUh2OsYt57DkePSTeiMeYblKra+WyCz3dAoT+hNcy/eIBvyyi"
"mTn95XpfsUL5UXGhHHzaDFn98y4PX9NpnoH9rfwt0cZ4uQ5cWtmYIlh3v0M3X8mSQsAHJJm6d28UUbaB"
"pMAFmO2BIxMEVD9QUpvQ1qoC30vv8Y27bX07ITuu6aI89l1dZbjQ/NECAwEAAQ==";
char vds::keys_control::common_news_read_private_key_[3137] =
"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"
"WzRE6+LyWw==";
char vds::keys_control::common_news_write_public_key_[4097] =
"MIICCgKCAgEAr8/vuGflWydiANI0na7s6MDw0oKwt1wzvjl0gO12kOyi3I+YzBhxdLwjmDNmeRErmmgN"
"EbYPlXEN3CVTBJsPtr1avGWD9VcVxQ5UmHaMcBVS+Abb+Tl2NCjRoEf3zJedxpsuQ+vz5Ow60I5/yuML"
"EXZW4k5WkaD8J+OF+ddPZ6i6In1Ya3FPpzi4ghebPixHDiQhwWTVB7CUQ2b4vkRQBroeqW6iyPChn7PT"
"NKJb2foGQ/hvpWmQVWoMjNJaat7U8cTJ92cNP23oBCss0WA9r476yvzQQuGdunKpmF04vaOQOU1ctyb2"
"uCmI68vSmTF1p+xl4aKtJq/rs7h0FEQit+4C1s0/co97jk/w22FOWPMNkoT0J8aGuq3IXFILCH5ktPj4"
"uQzs1/q1g+Vks0+JlPzuWB8AUGJokHM1/9AdVEVva8Kb8wP6g0f/Z9fpNVc32PsZPZAWLoSUcSKw8GPR"
"CPY4iR49iAG5WSt2Jb1ZkbVFYCyhrZ+9tWOyr5XZHa95sGIx7laTSEnbzg59lx+whUeKHVUCD5gc+wNd"
"XiCCmT4saKkRE39pU4a4eKkScJfCJ5ntal6OKEzD+Ziju+1XUEVEg6dhQjNaGrSyczW2X++KwNXI2F75"
"kKWgZa5QMmSD6uhnIdXDGZ3JP9uIuzNI+7lshJJobaxo63kggMc1PvsCAwEAAQ==";
char vds::keys_control::common_news_admin_public_key_[4097] =
"MIICCgKCAgEAvwaEDdspYfYjndo4shSp1WJLC/mmrx9S2qJnjNUPfDxlwwt77nd0NwWhf5CYgvuvAUiQ"
"RanJJK8fP3UUN+UDZWDsLSGhoqb17gxoZljnSWYhsrkjp/mJ3TFRgpucsmPvrWqbx7ZPMGBeexb90NRs"
"v9haSCcmGLasiezgelZi6l5IYtGEY2XXWnH1GTX7cdwTWqYfk9x3xBDEQBTgm4eCpe2gIe05sD5z+zPw"
"HEBn1r9vSnkEZXymwnkK7jT0Adlj7Y3fuZnsF4RaLyV3OHnXxEWySmPmTnF+ygFC3F0GGJaM13IOWBCH"
"aeGydvLTNdktq0O8RyMWB0JN8WlpjOgd3unOQLVI94MV8z4W1wr6GETOyWssVnWtuviRtSY5Yz65uWEX"
"mFo0QRKiDjdh3gmaHZxoeJOmUt43au4Lb2YA1iy7wAaytx4Jixi+Ha20PCLwwVYmdABqW7YH4qcybUkR"
"rHV0ISAKvbPnd3114/YDUfb00392tIpvQebTP9eOyLbsm2wqg/GRnojxGUL7/k/+tqmd7mXkALmtHqyT"
"jT0yRb4+hXbPgYKhom9YuQxcj7Y9aXoWaaYPv2d2vlt7xahQ6skgKEnDXkQj+2W8vmJPGdx/Bs27XsV7"
"ws2jGzPpfvj/KpWatoOnciNpPZh9rmfBWnSKpzjFvsFPkdmlbzjKkB8CAwEAAQ==";
char vds::keys_control::autoupdate_channel_id_[65] =
"VjPy3jfb6r/fDgRTCKzg/7CuEDdGJxOEK+3RHtu8JeA=";
char vds::keys_control::autoupdate_read_public_key_[4097] =
"MIICCgKCAgEArWrM1TEiFacN3VmrY0aQ3dLLHWui8TPi2GCDckVGe+heqBQqf25MSuTNoctVzJv8bqeT"
"thZ7r3W60KpDGosVw4JJvlRmYjHbC0yKQLol6aGG6zCdHGhxLQkLNq/o8weaTCEyN8xVK4LaX5HQTVat"
"aKcfo1Txp53/f2s6VZ4YDDZTty15NJ8W3OQFjSXp/KiONrGlDk+cGScf5R2HWvveIet4a0jAkeOV3GTf"
"Hilsarx5jGKae2AsHkllOE6S2pOMg5wmSWa8LZCS4E5Vk/utG8hBHFzw636PvQkpWJ9cyn4EZP78TT8k"
"7B9noWqxl9tHZA/xDKJuUM8v/uKoZAEdvVinAuJqWjoyBZ38j8DufStdLgCIYYuiP5l/43ktDZs14c7Q"
"9YNjDN4dYRCeSJmUNJ5QM9cGZYliHxqzREivOxmktNbrCqUWnk9xGmydtikB1dsfgTJS62nW+UZucdlK"
"SY0ljm/n2yrBXkH0sGbopftO6GnBqB09GzWSUocqf0rBlfSuAjBNQiOdyKAemuUVjd8UMRaC0pHtsA4I"
"aIxFxbX7JbHKKYIL5jm9P/gj+nsKQRY+FhBS//hpyZT/Ap9gYnlfTaWQkSLbwabfvP5w4Lp6WmTee0TH"
"JZfWbqsIekZLN6IMimn8AZoyF+c8GX1r8tgvTnWi6YUoaVQuIi4yIPMCAwEAAQ==";
char vds::keys_control::autoupdate_read_private_key_[3137] =
"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"
"atwtIDtRKg==";
char vds::keys_control::autoupdate_write_public_key_[4097] =
"MIICCgKCAgEAtgQtRSYoSSWtWxkVlITWmLXpMyn4WNppd79aaM+rqveSA6C5bF0hr7+TIt+U2F3hoyAO"
"1AFV1dX8qYHounhNiWbrp3RWaS7P/XL10/Z6YDtzjVRUXdYxza6U66+4zbT0quaaB6l9pcBGh7Q56opB"
"yLXjL3nJHj4SJxPnaA3VxhT8VCnpAPLGQnJtzzbmuVzsf9xEnT3JuvdQrN8IJnuw/WtBTPAlTkE+XcRx"
"AuQqUVwKsbbPi/H7vtIw+l58PDLwrTJEmPvIkR1j3cF8OPT/emH3u1sezxfBud46Y//HdebD812P14vu"
"BhhiAY4RDZGIiR+WcM5pOw4L2RZGlYSu8RjEYZV/qqEjgJ/BqWgB0GbJ2HEh/sWFFhmFUAgfdQEVDgKR"
"4nZgI0p8MO+z3xjt2nbDTPzfai0b/IgruuPdyn4icGXsb02XFGVe3+FhFm8ianUMD/QgpYffVRaR6cVy"
"r8LUTuXNA33bNg9v+Gn7auysEJu0dgvwJuHqBgFHv8K1MQPMqLaPTeGHfunuHylQNMobRnH/sVzyBlck"
"AXjxyl0hrpPhelJepdxKFmm1jpO7IU8/N1b8dfxhTgRidl60K2BU+QVkEreA4f+A9mUGB731bhLgWuvE"
"MNJ36Qfcwbt0cGoBjIPULdqxScaVDUAuTV0aSiMxSC+2+FoOGL7nUWUCAwEAAQ==";
char vds::keys_control::autoupdate_admin_public_key_[4097] =
"MIICCgKCAgEA4eKKd35WL7mPREAp3B2SvFZqNVgnJEaVykdlomJp+xOKSO/c+ljfwHsllmIFE1jlLlSl"
"PjXKQj9GDQn1grDxYnwVmcS2W5Kuj8Bu6pGCrbopNss0eyZJegz6rwcYBAXaVhlksRQAOi3vfcT221DB"
"l4ZsSRCMPQjp3peXesy3Erdn0pUu0kgjhC1Y2K/+gi9dG5yxKT/j4Q13Uk3cCYxjzCzFKL4yPzQnXY7k"
"kHh+9yGjEqx0Sp7BscXVE9HORCVsPAa0elbVGw4q19eVfrfuueJNv/Gwof/EzigBHruEG9pP1+8CEWn2"
"Anw605Ip8ryXIbzFFE1GC3UR0jpDZkziioIHzW0ylqAHCUm1rMbNHWyBp/zkmZCC+yW+zjcEg8yBSV77"
"Z55Tob2GzS42VxsNiECjIYmW5H+9/k8qMbqkFsfFRc4Kils2V0l3bTPYU26uZUqqBuZZerhB92N2TH9c"
"Q27sEQKwkx7p5H59VZl9tuAyA9oyOgsIJ9lKq2Hggau78dK/6ynoPyyCCo6anPql+WOcknolqDBcfdUI"
"Qq8ZOl6ktRtMCEkwIyHRJCFYE7rnsEOc4NVm8ugJ21ZNlClxHeGgVpv0PX+7zCaRLnAArctvHP4uLHVC"
"PN69w+BQfsGelWLo4I7pn7fxZ3nEID5zJM8N9Kf8D0XRMX6d3h9gF0MCAwEAAQ==";
char vds::keys_control::web_channel_id_[65] =
"/k6iHOZaqJNPVdJiZPY1Ehfwegl3KqfM6RpZ4zV3Kq8=";
char vds::keys_control::web_read_public_key_[4097] =
"MIICCgKCAgEAs5UeR3KNh9mUUHrOjoEL0zofcwdH40j03jKa5YEfmYyd8rlhlU+bnFDwMbrPTHDXKX2t"
"IZWQLjyyRhxndrdSWHpDdHbCgFNt3x4Lq8hBwtUHaWvwLQ0dDaEcyPcZ8imUhhoFqvKJocuEbXxi0d9v"
"EgaOYdAMyWfgusQOw+HH7wuS2rDUxBLBXcVJ9dIwu5LnUg5WPaRbIDUntbdrtUVxi7OhsxgaJXndP2d2"
"TYlNZveIaQCGGC/FpMfL9BYptiAYXAahnTUGOrlQttC9SkNBWtv0Zb72+pw/ES9CbGZTfUB+cgWym6Xn"
"t/ZsymWb7OEN6/SsW41pZYP+eIh2hfh/qPezamAU/2X1AcIT2eq+cBKFY9uMhWyWOn/3dRfnd9eSZuf/"
"8ZqDsGYjPBgtIkvL/bkD5j0F1L/2Sbo/I/7dRdVBjB8UyeRQYTtfZVRQxrHOX0fH6nh8U01Wo28Qy1+t"
"kQ8ef944ybHL8UwZU/xC4zTyRHJqvmAP+9CVlIGevOFtGNbpnTYJygoC5Z6jy0+r91tkaMdpjpkdlZC0"
"Lrjm8xIU/f1k3xQsdcxIqyN08DydTUDROPBzjgH3WbjXtN3K+KMuyt4L++XnxSrWQYJ2Cb4jIdsHagH2"
"tlETbvz91wdqqNGQL5ub4lEo6wnMfkOhhMVN/TXElCH5ape/YZOjSWUCAwEAAQ==";
char vds::keys_control::web_read_private_key_[3137] =
"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"
"4y1FOqYcW4pg";
char vds::keys_control::web_write_public_key_[4097] =
"MIICCgKCAgEA5YvUpDgpkRxsQjZSy/FkRvT/9ZxNqKgx8OtQhwgRXyCviCWk15JPMu3Cpubup68E9c41"
"DfwY8BgCFKfkyjKkJLesoIv2ftUD14t67LVJ7H94YlcVNJnblhHFoYFQ8R8mNjrjqtloXAzfT798WvmO"
"y9d7mUaDl22+Akza1TrZQetzq9/7AEPMDGUyZL7j/SFd0URJZMp5QG/RT+HfDwgsApecBt2T89qVO+5p"
"vxRIZ7Zt4vatHxWeYkgilgXdxaqeSR4MwhPMeTryKJObE8qVsUhpUrFkaJllCGqLCgOhnPmTlp8b+/lB"
"JOcYjQSX/b8s34Qit0HSVMgXx2uSRXLDcmtSJeu6gR0raXZ9kyYNQpeuyX99IBwoRWYHG0Cq2emli11k"
"sJdose9t33lmYVP/WpRG1QrJ1loJlxh/eVjJLNVCdY9jDLdSdZk/l3VI7Upo1R0fmnmBfdRaz7usE+Wq"
"ut+wLow85DLO+gXN0onbTF+7SziyZtngjlaVGKUEljUL+ajDSP6mU7yPNZfBYR8rlW2Y6fYr/Z0pWYPR"
"3kXsRkNKH0ycWhTuqJpA3KP3VnzyliSXCQKI5mC7j8qIZtVk57hhFGIM5zPNu9iqRYydIseWBYBZakLL"
"rBoJi9Jjkw132WwHOySi76x0ngcigEefTDf+6+duYFlE3Id6We8o3JMCAwEAAQ==";
char vds::keys_control::web_admin_public_key_[4097] =
"MIICCgKCAgEAseBpABppY7FETupZWwmoYehF3FakmnSaJMpb5Hxdd45PytQbg+1J0SqP0JMbtHYY6kk0"
"9zrKddWBIMm/P9ZIi3kuLbh3y5v/EaWz+PNqsKFfWpUcKS+2IITnPR9LOgkiEBORR9ob5MbWLR2WKRWI"
"wYL6Rc1naQN0k4jaqjVmzCdrTHDEY0rfk3uo3Ag+fLHKnzTlVso1aifkTRnxy6qBAy2lBzoxEdEQm19P"
"YPQgW8NsMMw/1qLZPvtkswW4Gxd3J4wl9gSfAAl9iVoDdCGMir98ooSJbgMKPt1hjii7Dmj9p9ljUtCI"
"Tw4HlB9lYFFPCKQaFMy5cB0kXgVixEVJbWvzZibK5dGywYo2yUhyv6jDz5UDQpXgRtvgggaAOUODLT8M"
"DvolLeibasvARTCLndDSaRc0j0YghScFQNsgxZElPdijoKw4P8UMpFGchbyqY+XCQT4kVmJeLGEO/gFy"
"gAfRIjCX2szTL7lFmAYAP/CNbuWvTHCemjde2jBbfKrERK/23OswLmCPxqHT2XzYnklIxiXc0YSproIJ"
"lWTTumApiqzxt0ERgsfnx+Zqu9CmaazeZaeXDnaEwiz+2uigBudKDz3vabxuHUMQBzCTk1xbYVKT2Pys"
"t8zUtLPShUFZDu3PvXPAOs/VNXdzYBfv8HWrqLKW4Ql32/Cf5EGy9gkCAwEAAQ==";

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
