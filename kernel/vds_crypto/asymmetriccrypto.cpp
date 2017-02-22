/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "asymmetriccrypto.h"
#include "crypto_exception.h"

vds::asymmetric_private_key::asymmetric_private_key()
  : info_(vds::asymmetric_crypto::unknown()),
  ctx_(nullptr),
  key_(nullptr)
{
}

vds::asymmetric_private_key::asymmetric_private_key(EVP_PKEY * key)
  : info_(vds::asymmetric_crypto::unknown()),
  ctx_(nullptr),
  key_(key)
{
}

vds::asymmetric_private_key::asymmetric_private_key(asymmetric_private_key && original)
: info_(original.info_),
  ctx_(original.ctx_),
  key_(original.key_)
{
  original.ctx_ = nullptr;
  original.key_ = nullptr;
}

vds::asymmetric_private_key::asymmetric_private_key(
  const vds::asymmetric_crypto_info & info)
: info_(info), key_(nullptr)
{
  this->ctx_ = EVP_PKEY_CTX_new_id(info.id, NULL);
  if(nullptr == this->ctx_) {
    throw new std::runtime_error("Unable to create RSA context");
  }
}

vds::asymmetric_private_key::~asymmetric_private_key()
{
  if (nullptr != this->ctx_) {
    EVP_PKEY_CTX_free(this->ctx_);
  }
  else if (nullptr != this->key_) {
    EVP_PKEY_free(this->key_);
  }
}


void vds::asymmetric_private_key::generate()
{
  if (0 >= EVP_PKEY_keygen_init(this->ctx_)) {
    throw new std::runtime_error("Unable to init RSA context");
  }
  
  if (0 >= EVP_PKEY_CTX_set_rsa_keygen_bits(this->ctx_, this->info_.key_bits)) {
    throw new std::runtime_error("Unable to set RSA bits");
  }
  
  if (0 >= EVP_PKEY_keygen(this->ctx_, &this->key_)) {
    throw new std::runtime_error("Unable to generate RSA key");
  }
}

vds::asymmetric_private_key vds::asymmetric_private_key::parse(const std::string & value, const std::string & password)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), value.length());
  auto key = PEM_read_bio_PrivateKey(io, 0, 0, password.empty() ? nullptr : (void *)password.c_str());
  return asymmetric_private_key(key);
}

std::string vds::asymmetric_private_key::str(const std::string & password/* = std::string()*/) const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PrivateKey(bio, this->key_, NULL, NULL, 0, NULL, password.empty() ? nullptr : (void *)password.c_str());

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len + 1);
  BIO_read(bio, const_cast<char *>(result.data()), len);
  BIO_free_all(bio);

  return result;
}

void vds::asymmetric_private_key::load(const filename & filename, const std::string & password/* = std::string()*/)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if (nullptr == in) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to private key " + filename.str(), error);
  }

  RSA * r = PEM_read_bio_RSAPrivateKey(in, NULL, NULL, password.empty() ? nullptr : (void *)password.c_str());
  if (nullptr == r) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to read file key " + filename.str(), error);
  }

  this->key_ = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(this->key_, r);

  BIO_free(in);
}

void vds::asymmetric_private_key::save(const filename & filename, const std::string & password/* = std::string()*/)
{
  auto outf = BIO_new_file(filename.local_name().c_str(), "w");
  if (nullptr == outf) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed create file key " + filename.str(), error);
  }

  int r = PEM_write_bio_PrivateKey(outf, this->key_, NULL, NULL, 0, NULL, password.empty() ? nullptr : (void *)password.c_str());
  if (0 == r) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed save private key " + filename.str(), error);
  }

  BIO_free(outf);
}

const vds::asymmetric_crypto_info & vds::asymmetric_crypto::unknown()
{
  static asymmetric_crypto_info result = {
    EVP_PKEY_NONE,
    -1
  };

  return result;
}

const vds::asymmetric_crypto_info & vds::asymmetric_crypto::rsa2048()
{
  static asymmetric_crypto_info result = {
    EVP_PKEY_RSA,
    2048
  };

  return result;
}

const vds::asymmetric_crypto_info & vds::asymmetric_crypto::rsa4096()
{
  static asymmetric_crypto_info result = {
    EVP_PKEY_RSA,
    4096
  };

  return result;
}

vds::asymmetric_sign::asymmetric_sign(const hash_info & hash_info, const asymmetric_private_key & key)
  : md_(nullptr), sig_(nullptr)
{
  this->ctx_ = EVP_MD_CTX_create();
  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_MD_CTX_create", error);
  }

  this->md_ = EVP_get_digestbynid(hash_info.id);
  if (nullptr == this->md_) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_get_digestbynid", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, this->md_, NULL)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }

  if (1 != EVP_DigestSignInit(this->ctx_, NULL, this->md_, NULL, key.key_)){
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }
}

vds::asymmetric_sign::~asymmetric_sign()
{
  if (nullptr != this->sig_) {
    OPENSSL_free(this->sig_);
  }

  if (this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::asymmetric_sign::update(const void * data, int len)
{
  if (1 != EVP_DigestSignUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }
}

void vds::asymmetric_sign::final()
{
  size_t req = 0;
  if(1 != EVP_DigestSignFinal(this->ctx_, NULL, &req) || req <= 0) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestSignFinal", error);
  }

  this->sig_ = (unsigned char *)OPENSSL_malloc(req);
  if (nullptr == this->sig_) {
    auto error = ERR_get_error();
    throw new crypto_exception("OPENSSL_malloc", error);
  }

  this->sig_len_ = req;
  if (1 != EVP_DigestSignFinal(this->ctx_, this->sig_, &this->sig_len_)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestSignFinal", error);
  }

  if (this->sig_len_ != req) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestSignFinal", error);
  }
}

vds::asymmetric_sign_verify::asymmetric_sign_verify(const hash_info & hash_info, const asymmetric_public_key & key)
  : md_(nullptr)
{
  this->ctx_ = EVP_MD_CTX_create();
  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_MD_CTX_create", error);
  }

  this->md_ = EVP_get_digestbynid(hash_info.id);
  if (nullptr == this->md_) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_get_digestbynid", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, this->md_, NULL)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }

  if (1 != EVP_DigestVerifyInit(this->ctx_, NULL, this->md_, NULL, key.key_)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }
}

vds::asymmetric_sign_verify::~asymmetric_sign_verify()
{
  if (this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::asymmetric_sign_verify::update(const void * data, int len)
{
  if (1 != EVP_DigestVerifyUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }
}

bool vds::asymmetric_sign_verify::verify(const unsigned char * sig, size_t sig_len)
{
  return 1 == EVP_DigestVerifyFinal(this->ctx_, const_cast<unsigned char *>(sig), sig_len);
}

vds::asymmetric_public_key::asymmetric_public_key(EVP_PKEY * key)
  : info_(asymmetric_crypto::unknown()), key_(key)
{
}

vds::asymmetric_public_key::asymmetric_public_key(asymmetric_public_key && original)
  : info_(original.info_), key_(original.key_)
{
  original.key_ = nullptr;
}

vds::asymmetric_public_key::asymmetric_public_key(const asymmetric_private_key & key)
  : info_(key.info_), key_(nullptr)
{
  auto len = i2d_PUBKEY(key.key_, NULL);
  unsigned char * buf = (unsigned char *)OPENSSL_malloc(len);

  unsigned char * p = buf;
  len = i2d_PUBKEY(key.key_, &p);

  const unsigned char * p1 = buf;
  this->key_ = d2i_PUBKEY(NULL, &p1, len);

  if (nullptr == this->key_) {
    auto error = ERR_get_error();
    OPENSSL_free(buf);

    throw new crypto_exception("d2i_PUBKEY", error);
  }
}

vds::asymmetric_public_key::~asymmetric_public_key()
{
}

vds::asymmetric_public_key vds::asymmetric_public_key::parse(const std::string & value)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), value.length());
  auto key = PEM_read_bio_PUBKEY(io, 0, 0, 0);
  return asymmetric_public_key(key);
}

std::string vds::asymmetric_public_key::str() const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(bio, this->key_);

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len + 1);
  BIO_read(bio, const_cast<char *>(result.data()), len);
  BIO_free_all(bio);

  return result;
}

void vds::asymmetric_public_key::load(const filename & filename)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if (nullptr == in) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to public key " + filename.str(), error);
  }

  RSA * r = PEM_read_bio_RSAPublicKey(in, NULL, NULL, NULL);
  if (nullptr == r) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to read public file key " + filename.str(), error);
  }

  this->key_ = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(this->key_, r);

  BIO_free(in);
}

void vds::asymmetric_public_key::save(const filename & filename)
{
  auto outf = BIO_new_file(filename.local_name().c_str(), "w");
  if (nullptr == outf) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed create file key " + filename.str(), error);
  }

  int r = PEM_write_bio_PUBKEY(outf, this->key_);
  if (0 == r) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed save private key " + filename.str(), error);
  }

  BIO_free(outf);
}

vds::certificate::certificate()
: cert_(nullptr)
{
}

vds::certificate::certificate(certificate && original)
  : cert_(original.cert_)
{
  original.cert_ = nullptr;
}

vds::certificate::certificate(X509 * cert)
  : cert_(cert)
{
}

vds::certificate::~certificate()
{
  if(nullptr != this->cert_){
    X509_free(this->cert_);
  }
}

vds::certificate vds::certificate::parse(const std::string & value)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), value.length());
  auto cert = PEM_read_bio_X509(io, 0, 0, 0);
  return certificate(cert);
}

std::string vds::certificate::str() const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(bio, this->cert_);

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len + 1);
  BIO_read(bio, const_cast<char *>(result.data()), len);
  BIO_free_all(bio);

  return result;
}

void vds::certificate::load(const filename & filename)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if(nullptr == in){
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to load certificate " + filename.str(), error);
  }

  this->cert_ = PEM_read_bio_X509(in, NULL, NULL, NULL);
  if(nullptr == this->cert_){
    auto error = ERR_get_error();
    BIO_free(in);
    throw new crypto_exception("Failed to load certificate " + filename.str(), error);
  }
  
  BIO_free(in);
}

void vds::certificate::save(const filename & filename)
{
  auto out = BIO_new_file(filename.local_name().c_str(), "w");
  auto ret = PEM_write_bio_X509(out, this->cert_);
  BIO_free_all(out);

  if (1 != ret) {
    auto error = ERR_get_error();
    throw new crypto_exception("PEM_write_bio_X509", error);
  }
}

std::string vds::certificate::subject() const
{
  char result[1024];
  X509_NAME_oneline(X509_get_subject_name(this->cert_), result, sizeof(result));

  return result;
}

std::string vds::certificate::issuer() const
{
  char result[1024];
  X509_NAME_oneline(X509_get_issuer_name(this->cert_), result, sizeof(result));

  return result;
}

std::string vds::certificate::fingerprint(const vds::hash_info & hash_algo) const
{
  unsigned char md[EVP_MAX_MD_SIZE];
  unsigned int n;
  if(!X509_digest(this->cert_, hash_algo.type, md, &n)){
    auto error = ERR_get_error();
    throw new crypto_exception("X509_digest", error);
  }
  
  return base64::from_bytes(md, n);
}


vds::certificate vds::certificate::create_new(
  const asymmetric_public_key & new_certificate_public_key,
  const asymmetric_private_key & new_certificate_private_key,
  const create_options & options
)
{
  X509 * x509 = X509_new();
  if (nullptr == x509) {
    auto error = ERR_get_error();
    throw new crypto_exception("X509_new", error);
  }

  try {

    //if (!EVP_PKEY_assign_RSA(new_certificate_private_key.key(), x509)) {
    //  auto error = ERR_get_error();
    //  throw new crypto_exception("X509_new", error);
    //}

    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), (long)60 * 60 * 24 * 365);
    X509_set_pubkey(x509, new_certificate_public_key.key());

    auto name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char *)options.country.c_str(), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char *)options.organization.c_str(), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char *)options.name.c_str(), -1, -1, 0);

    for(auto& extension : options.extensions) {
      int nid = OBJ_create(extension.oid.c_str(), extension.name.c_str(), extension.description.c_str());
      X509V3_EXT_add_alias(nid, NID_netscape_comment);
      add_ext(x509, nid, "example comment alias");
    }

    if (nullptr == options.ca_certificate) {
      X509_set_issuer_name(x509, name);
    }
    else {
      X509_set_issuer_name(x509, X509_get_subject_name(options.ca_certificate->cert()));
    }

    /* Add various extensions: standard extensions */
    add_ext(x509, NID_basic_constraints, "critical,CA:TRUE");
    add_ext(x509, NID_key_usage, "critical,keyCertSign,cRLSign");
    add_ext(x509, NID_subject_key_identifier, "hash");

    /* Some Netscape specific extensions */
    add_ext(x509, NID_netscape_cert_type, "sslCA");
    add_ext(x509, NID_netscape_comment, "example comment extension");

    if (nullptr == options.ca_certificate) {
      if (!X509_sign(x509, new_certificate_private_key.key(), EVP_sha256())) {
        auto error = ERR_get_error();
        throw new crypto_exception("X509_new", error);
      }
    }
    else {
      if (!X509_sign(x509, options.ca_certificate_private_key->key(), EVP_sha256())) {
        auto error = ERR_get_error();
        throw new crypto_exception("X509_new", error);
      }
    }

    return certificate(x509);
  }
  catch (...) {
    X509_free(x509);
    throw;
  }
}

vds::asymmetric_public_key vds::certificate::public_key() const
{
  auto key = X509_get_pubkey(this->cert_);
  if (nullptr == key) {
    auto error = ERR_get_error();
    throw new crypto_exception("X509_get_pubkey", error);
  }
  return asymmetric_public_key(key);
}

bool vds::certificate::is_ca_cert() const
{
  return (0 < X509_check_ca(this->cert_));
}

bool vds::certificate::is_issued(const vds::certificate& issuer) const
{
  return (X509_V_OK == X509_check_issued(issuer.cert(), this->cert()));
}


bool vds::certificate::add_ext(X509 * cert, int nid, const char * value)
{
  X509V3_CTX ctx;
  X509V3_set_ctx_nodb(&ctx);

  X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);

  X509_EXTENSION * ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, const_cast<char *>(value));
  if (nullptr == ex) {
    return false;
  }

  X509_add_ext(cert, ex, -1);
  X509_EXTENSION_free(ex);

  return true;
}

vds::certificate_extension::certificate_extension()
  : base_nid(NID_netscape_comment)
{
}

vds::certificate_store::certificate_store()
: store_(X509_STORE_new())
{
  if (nullptr == this->store_) {
    auto error = ERR_get_error();
    throw new crypto_exception("X509_get_pubkey", error);
  }
}

vds::certificate_store::~certificate_store()
{
  if (nullptr != this->store_) {
    X509_STORE_free(this->store_);
  }
}

void vds::certificate_store::add(const vds::certificate& cert)
{
  if(0 >= X509_STORE_add_cert(this->store_, cert.cert())){
    auto error = ERR_get_error();
    throw new crypto_exception("unable to add certificate to store", error);
  }
}


void vds::certificate_store::load_locations(const std::string & location)
{
  if(0 >= X509_STORE_load_locations(this->store_, location.c_str(), NULL)){
    auto error = ERR_get_error();
    throw new crypto_exception("unable to load certificates at " + location + " to store", error);
  }
}

vds::certificate_store::verify_result vds::certificate_store::verify(const vds::certificate& cert) const
{
  X509_STORE_CTX * vrfy_ctx = X509_STORE_CTX_new();
  if (nullptr == vrfy_ctx) {
    auto error = ERR_get_error();
    throw new crypto_exception("X509_STORE_CTX_new", error);
  }

  try {

    X509_STORE_CTX_init(vrfy_ctx, this->store_, cert.cert(), NULL);

    verify_result result;
    if (0 == X509_verify_cert(vrfy_ctx)) {
      result.error_code = X509_STORE_CTX_get_error(vrfy_ctx);
      result.error = X509_verify_cert_error_string(result.error_code);
      auto error_cert = X509_STORE_CTX_get_current_cert(vrfy_ctx);
      
      char issuer[1024];
      X509_NAME_oneline(X509_get_issuer_name(error_cert), issuer, sizeof(issuer));
      
      result.issuer = issuer;
    }
    else {
      result.error_code = 0;
    }

    X509_STORE_CTX_free(vrfy_ctx);
    
    return result;
  }
  catch (...) {
    X509_STORE_CTX_free(vrfy_ctx);

    throw;
  }
}


