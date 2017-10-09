/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "asymmetriccrypto.h"
#include "private/asymmetriccrypto_p.h"
#include "crypto_exception.h"
#include "private/hash_p.h"
#include "symmetriccrypto.h"

///////////////////////////////////////////////////////////////////////
vds::asymmetric_private_key::asymmetric_private_key()
: impl_(new _asymmetric_private_key())
{
}

vds::asymmetric_private_key::asymmetric_private_key(const asymmetric_private_key & original)
  : impl_(original.impl_)
{
}

vds::asymmetric_private_key::asymmetric_private_key(
  const vds::asymmetric_crypto_info & info)
: impl_(new _asymmetric_private_key(info))
{
}

vds::asymmetric_private_key::asymmetric_private_key(_asymmetric_private_key * impl)
: impl_(impl)
{
}

vds::asymmetric_private_key::~asymmetric_private_key()
{
}

void vds::asymmetric_private_key::generate()
{
  this->impl_->generate();
}

vds::asymmetric_private_key vds::asymmetric_private_key::parse(const std::string & value, const std::string & password)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), (int)value.length());
  auto key = PEM_read_bio_PrivateKey(io, 0, 0, password.empty() ? nullptr : (void *)password.c_str());
  return asymmetric_private_key(new _asymmetric_private_key(key));
}

std::string vds::asymmetric_private_key::str(const std::string & password/* = std::string()*/) const
{
  return this->impl_->str(password);
}

vds::const_data_buffer vds::asymmetric_private_key::der(const service_provider & sp, const std::string & password /*= std::string()*/) const
{
  return this->impl_->der(sp, password);
}

vds::asymmetric_private_key vds::asymmetric_private_key::parse_der(
  const service_provider & sp,
  const const_data_buffer & value,
  const std::string & password /*= std::string()*/)
{
  return _asymmetric_private_key::parse_der(sp, value, password);
}


void vds::asymmetric_private_key::load(const filename & filename, const std::string & password/* = std::string()*/)
{
  this->impl_->load(filename, password);
}

void vds::asymmetric_private_key::save(const filename & filename, const std::string & password/* = std::string()*/) const
{
  this->impl_->save(filename, password);
}

vds::const_data_buffer vds::asymmetric_private_key::decrypt(const const_data_buffer & data) const
{
  return this->impl_->decrypt(data);
}

///////////////////////////////////////////////////////////////////////
vds::_asymmetric_private_key::_asymmetric_private_key()
  : info_(vds::asymmetric_crypto::unknown()),
  ctx_(nullptr),
  key_(nullptr)
{
}

vds::_asymmetric_private_key::_asymmetric_private_key(EVP_PKEY * key)
  : info_(vds::asymmetric_crypto::unknown()),
  ctx_(nullptr),
  key_(key)
{
}

vds::_asymmetric_private_key::_asymmetric_private_key(
  const vds::asymmetric_crypto_info & info)
: info_(info), key_(nullptr)
{
  this->ctx_ = EVP_PKEY_CTX_new_id(info.id, NULL);
  if(nullptr == this->ctx_) {
    throw std::runtime_error("Unable to create RSA context");
  }
}

vds::_asymmetric_private_key::~_asymmetric_private_key()
{
  if (nullptr != this->ctx_) {
    EVP_PKEY_CTX_free(this->ctx_);
  }
  else if (nullptr != this->key_) {
    EVP_PKEY_free(this->key_);
  }
}


void vds::_asymmetric_private_key::generate()
{
  if (0 >= EVP_PKEY_keygen_init(this->ctx_)) {
    throw std::runtime_error("Unable to init RSA context");
  }
  
  if (0 >= EVP_PKEY_CTX_set_rsa_keygen_bits(this->ctx_, this->info_.key_bits)) {
    throw std::runtime_error("Unable to set RSA bits");
  }
  
  if (0 >= EVP_PKEY_keygen(this->ctx_, &this->key_)) {
    throw std::runtime_error("Unable to generate RSA key");
  }
}

std::string vds::_asymmetric_private_key::str(const std::string & password/* = std::string()*/) const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PrivateKey(bio, this->key_, NULL, NULL, 0, NULL, password.empty() ? nullptr : (void *)password.c_str());

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len);
  if (len != BIO_read(bio, const_cast<char *>(result.data()), len)) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to BIO_read", error);
  }
  BIO_free_all(bio);

  return result;
}

vds::const_data_buffer vds::_asymmetric_private_key::der(
  const service_provider & sp,
  const std::string & password/* = std::string()*/) const
{
  auto len = i2d_PrivateKey(this->key_, NULL);

  auto buf = (unsigned char *)OPENSSL_malloc(len);
  if (NULL == buf) {
    throw std::runtime_error("Out of memory at get DER format of certificate");
  }

  auto p = buf;
  i2d_PrivateKey(this->key_, &p);

  if(!password.empty()){
    std::vector<uint8_t> buffer;

    auto key = symmetric_key::from_password(password);
    auto result = symmetric_encrypt::encrypt(key, buf, len);
    
    OPENSSL_free(buf);
    return result;
  }
  else {
    const_data_buffer result(buf, len);
    OPENSSL_free(buf);

    return result;
  }
}

vds::asymmetric_private_key vds::_asymmetric_private_key::parse_der(
  const service_provider & sp,
  const const_data_buffer & value,
  const std::string & password /*= std::string()*/)
{
  if(!password.empty()){
    auto key = symmetric_key::from_password(password);
    auto buffer = symmetric_decrypt::decrypt(
      key,
      value.data(),
      value.size());
    
    const unsigned char * p = buffer->data();
    auto key = d2i_AutoPrivateKey(NULL, &p, buffer->size());
    return asymmetric_private_key(new _asymmetric_private_key(key));
  }
  else{
      const unsigned char * p = value.data();
      auto key = d2i_AutoPrivateKey(NULL, &p, value.size());
      return asymmetric_private_key(new _asymmetric_private_key(key));
  }
}

void vds::_asymmetric_private_key::load(const filename & filename, const std::string & password/* = std::string()*/)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if (nullptr == in) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to private key " + filename.str(), error);
  }

  RSA * r = PEM_read_bio_RSAPrivateKey(in, NULL, NULL, password.empty() ? nullptr : (void *)password.c_str());
  if (nullptr == r) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to read file key " + filename.str(), error);
  }

  this->key_ = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(this->key_, r);

  BIO_free(in);
}

void vds::_asymmetric_private_key::save(const filename & filename, const std::string & password/* = std::string()*/) const
{
  auto outf = BIO_new_file(filename.local_name().c_str(), "w");
  if (nullptr == outf) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed create file key " + filename.str(), error);
  }

  int r = PEM_write_bio_PrivateKey(outf, this->key_, NULL, NULL, 0, NULL, password.empty() ? nullptr : (void *)password.c_str());
  if (0 == r) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed save private key " + filename.str(), error);
  }

  BIO_free(outf);
}
////////////////////////////////////////////////////////////////////////////////
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

vds::asymmetric_sign::asymmetric_sign(const hash_info & hash_info, const asymmetric_private_key & key, const_data_buffer & signature)
: hash_info_(hash_info), key_(key), signature_(signature)
{
}

vds::_asymmetric_sign * vds::asymmetric_sign::create_implementation() const
{
  return new _asymmetric_sign(this->hash_info_, this->key_);
}

void vds::asymmetric_sign::data_update(_asymmetric_sign * impl, const uint8_t * data, int len)
{
  impl->update(data, len);
}

void vds::asymmetric_sign::data_final(_asymmetric_sign * impl, const_data_buffer & result)
{
  impl->final();
  result = impl->signature();
}

vds::const_data_buffer vds::asymmetric_sign::signature(
  const hash_info & hash_info,
  const asymmetric_private_key & key,
  const const_data_buffer & data)
{
  return signature(
    hash_info,
    key,
    data.data(),
    data.size());
}

vds::const_data_buffer vds::asymmetric_sign::signature(
  const vds::hash_info& hash_info,
  const vds::asymmetric_private_key& key,
  const void* data,
  size_t data_size)
{
  _asymmetric_sign s(hash_info, key);
  s.update(data, data_size);
  s.final();
  return s.signature();
}


////////////////////////////////////////////////////////////////////////////////
vds::_asymmetric_sign::_asymmetric_sign(const hash_info & hash_info, const asymmetric_private_key & key)
  : md_(nullptr)
{
  this->ctx_ = EVP_MD_CTX_create();
  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_MD_CTX_create", error);
  }

  this->md_ = EVP_get_digestbynid(hash_info.id);
  if (nullptr == this->md_) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_get_digestbynid", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, this->md_, NULL)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }

  if (1 != EVP_DigestSignInit(this->ctx_, NULL, this->md_, NULL, key.impl_->key_)){
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }
}

vds::_asymmetric_sign::~_asymmetric_sign()
{
  if (this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::_asymmetric_sign::update(const void * data, int len)
{
  if (1 != EVP_DigestSignUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }
}

void vds::_asymmetric_sign::final()
{
  size_t req = 0;
  if(1 != EVP_DigestSignFinal(this->ctx_, NULL, &req) || req <= 0) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestSignFinal", error);
  }

  auto sig = (unsigned char *)OPENSSL_malloc(req);
  if (nullptr == sig) {
    auto error = ERR_get_error();
    throw crypto_exception("OPENSSL_malloc", error);
  }

  auto len = req;
  if (1 != EVP_DigestSignFinal(this->ctx_, sig, &len)) {
    auto error = ERR_get_error();
    OPENSSL_free(sig);
    throw crypto_exception("EVP_DigestSignFinal", error);
  }

  if (len != req) {
    auto error = ERR_get_error();
    OPENSSL_free(sig);
    throw crypto_exception("EVP_DigestSignFinal", error);
  }
  
  this->sig_.reset(sig, len);
  OPENSSL_free(sig);
}
///////////////////////////////////////////////////////////////
vds::asymmetric_sign_verify::asymmetric_sign_verify(
  const hash_info & hash_info,
  const asymmetric_public_key & key,
  const const_data_buffer & sign)
: hash_info_(hash_info), key_(key), signature_(sign)
{
}

vds::_asymmetric_sign_verify * vds::asymmetric_sign_verify::create_implementation() const
{
  return new _asymmetric_sign_verify(this->hash_info_, this->key_);
}

void vds::asymmetric_sign_verify::data_update(_asymmetric_sign_verify * impl, const void * data, int len)
{
  impl->update(data, len);
}

bool vds::asymmetric_sign_verify::data_final(_asymmetric_sign_verify * impl, const const_data_buffer & signature)
{
  return impl->verify(signature);
}

bool vds::asymmetric_sign_verify::verify(
  const vds::hash_info& hash_info,
  const vds::asymmetric_public_key& key,
  const const_data_buffer& signature,
  const void* data,
  size_t data_size)
{
  _asymmetric_sign_verify s(hash_info, key);
  s.update(data, data_size);
  return s.verify(signature);
}

bool vds::asymmetric_sign_verify::verify(
  const hash_info & hash_info,
  const asymmetric_public_key & key,
  const const_data_buffer & signature,
  const const_data_buffer & data)
{
  return verify(hash_info, key, signature, data.data(), data.size());
}

///////////////////////////////////////////////////////////////
vds::_asymmetric_sign_verify::_asymmetric_sign_verify(const hash_info & hash_info, const asymmetric_public_key & key)
  : md_(nullptr)
{
  this->ctx_ = EVP_MD_CTX_create();
  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_MD_CTX_create", error);
  }

  this->md_ = EVP_get_digestbynid(hash_info.id);
  if (nullptr == this->md_) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_get_digestbynid", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, this->md_, NULL)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }

  if (1 != EVP_DigestVerifyInit(this->ctx_, NULL, this->md_, NULL, key.impl_->key_)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }
}

vds::_asymmetric_sign_verify::~_asymmetric_sign_verify()
{
  if (this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::_asymmetric_sign_verify::update(const void * data, int len)
{
  if (1 != EVP_DigestVerifyUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }
}

bool vds::_asymmetric_sign_verify::verify(const const_data_buffer & sig)
{
  return 1 == EVP_DigestVerifyFinal(this->ctx_, const_cast<unsigned char *>(sig.data()), sig.size());
}
//////////////////////////////////////////////////////////////////////
vds::asymmetric_public_key::asymmetric_public_key(_asymmetric_public_key * impl)
: impl_(impl)
{
}

vds::asymmetric_public_key::asymmetric_public_key(const asymmetric_public_key & original)
  : impl_(original.impl_)
{
}

vds::asymmetric_public_key::asymmetric_public_key(const asymmetric_private_key & key)
  : impl_(new _asymmetric_public_key(key))
{
}

vds::asymmetric_public_key::~asymmetric_public_key()
{
}

vds::asymmetric_public_key vds::asymmetric_public_key::parse(const std::string & value)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), (int)value.length());
  auto key = PEM_read_bio_PUBKEY(io, 0, 0, 0);
  return asymmetric_public_key(new _asymmetric_public_key(key));
}

std::string vds::asymmetric_public_key::str() const
{
  return this->impl_->str();
}

void vds::asymmetric_public_key::load(const filename & filename)
{
  this->impl_->load(filename);
}

void vds::asymmetric_public_key::save(const filename & filename)
{
  this->impl_->save(filename);
}

vds::const_data_buffer vds::asymmetric_public_key::encrypt(const const_data_buffer & data)
{
  return this->impl_->encrypt(data.data(), data.size());
}

vds::const_data_buffer vds::asymmetric_public_key::encrypt(const void * data, size_t data_size)
{
  return this->impl_->encrypt(data, data_size);
}

//////////////////////////////////////////////////////////////////////
vds::_asymmetric_public_key::_asymmetric_public_key(EVP_PKEY * key)
  : info_(asymmetric_crypto::unknown()), key_(key)
{
}

vds::_asymmetric_public_key::_asymmetric_public_key(const asymmetric_private_key & key)
  : info_(key.impl_->info_), key_(nullptr)
{
  auto len = i2d_PUBKEY(key.impl_->key_, NULL);
  unsigned char * buf = (unsigned char *)OPENSSL_malloc(len);

  unsigned char * p = buf;
  len = i2d_PUBKEY(key.impl_->key_, &p);

  const unsigned char * p1 = buf;
  this->key_ = d2i_PUBKEY(NULL, &p1, len);

  if (nullptr == this->key_) {
    auto error = ERR_get_error();
    OPENSSL_free(buf);

    throw crypto_exception("d2i_PUBKEY", error);
  }
}

vds::_asymmetric_public_key::~_asymmetric_public_key()
{
  if(nullptr != this->key_){
    EVP_PKEY_free(this->key_);
  }
}


std::string vds::_asymmetric_public_key::str() const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_PUBKEY(bio, this->key_);

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len);
  if (len != BIO_read(bio, const_cast<char *>(result.data()), len)) {
    auto error = ERR_get_error();
    BIO_free_all(bio);
    throw crypto_exception("EVP_DigestInit_ex", error);
  }

  BIO_free_all(bio);

  return result;
}

void vds::_asymmetric_public_key::load(const filename & filename)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if (nullptr == in) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to public key " + filename.str(), error);
  }

  RSA * r = PEM_read_bio_RSAPublicKey(in, NULL, NULL, NULL);
  if (nullptr == r) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to read public file key " + filename.str(), error);
  }

  this->key_ = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(this->key_, r);

  BIO_free(in);
}

void vds::_asymmetric_public_key::save(const filename & filename) const
{
  auto outf = BIO_new_file(filename.local_name().c_str(), "w");
  if (nullptr == outf) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed create file key " + filename.str(), error);
  }

  int r = PEM_write_bio_PUBKEY(outf, this->key_);
  if (0 == r) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed save private key " + filename.str(), error);
  }

  BIO_free(outf);
}

/////////////////////////////////////////////////////////////////////////////
vds::certificate::certificate()
: impl_(new _certificate())
{
}

vds::certificate::certificate(_certificate * impl)
: impl_(impl)
{
}

vds::certificate::certificate(const certificate & original)
  : impl_(original.impl_)
{
}

vds::certificate::~certificate()
{
}

vds::certificate vds::certificate::parse(const std::string & value)
{
  auto io = BIO_new_mem_buf((void*)value.c_str(), (int)value.length());
  auto cert = PEM_read_bio_X509(io, 0, 0, 0);
  if(nullptr == cert){
    throw std::runtime_error("Invalid certificate format");
  }
  return certificate(new _certificate(cert));
}

std::string vds::certificate::str() const
{
  return this->impl_->str();
}

vds::certificate vds::certificate::parse_der(const const_data_buffer & value)
{
  auto p = value.data();
  auto cert = d2i_X509(NULL, &p, value.size());

  if (NULL == cert) {
    auto error = ERR_get_error();
    throw crypto_exception("Failed to parse certificate from DER", error);
  }

  return certificate(new _certificate(cert));
}

vds::const_data_buffer vds::certificate::der() const
{
  return this->impl_->der();
}

void vds::certificate::load(const filename & filename)
{
  this->impl_->load(filename);
}

void vds::certificate::save(const filename & filename) const
{
  this->impl_->save(filename);
}

std::string vds::certificate::subject() const
{
  return this->impl_->subject();
}

std::string vds::certificate::issuer() const
{
  return this->impl_->issuer();
}

vds::const_data_buffer vds::certificate::fingerprint(const vds::hash_info & hash_algo) const
{
  return this->impl_->fingerprint(hash_algo);
}

vds::certificate vds::certificate::create_new(
  const asymmetric_public_key & new_certificate_public_key,
  const asymmetric_private_key & new_certificate_private_key,
  const create_options & options
)
{
  return _certificate::create_new(
    new_certificate_public_key,
    new_certificate_private_key,
    options);
}

vds::asymmetric_public_key vds::certificate::public_key() const
{
  return this->impl_->public_key();
}

bool vds::certificate::is_ca_cert() const
{
  return this->impl_->is_ca_cert();
}

bool vds::certificate::is_issued(const vds::certificate& issuer) const
{
  return this->impl_->is_issued(issuer);
}

int vds::certificate::extension_count() const
{
  return this->impl_->extension_count();
}

int vds::certificate::extension_by_NID(int nid) const
{
  return this->impl_->extension_by_NID(nid);
}

vds::certificate_extension vds::certificate::get_extension(int index) const
{
  return this->impl_->get_extension(index);
}

vds::certificate & vds::certificate::operator = (const certificate & original)
{
  this->impl_ = original.impl_;
  return *this;
}

/////////////////////////////////////////////////////////////////////////////
vds::_certificate::_certificate()
: cert_(nullptr)
{
}

vds::_certificate::_certificate(X509 * cert)
  : cert_(cert)
{
}

vds::_certificate::~_certificate()
{
  if(nullptr != this->cert_){
    X509_free(this->cert_);
  }
}

std::string vds::_certificate::str() const
{
  BIO * bio = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(bio, this->cert_);

  auto len = BIO_pending(bio);
  std::string result;
  result.resize(len);
  BIO_read(bio, const_cast<char *>(result.data()), len);
  BIO_free_all(bio);

  return result;
}

vds::const_data_buffer vds::_certificate::der() const
{
  auto len = i2d_X509(this->cert_, NULL);

  auto buf = (unsigned char *)OPENSSL_malloc(len);
  if (NULL == buf) {
    throw std::runtime_error("Out of memory at get DER format of certificate");
  }

  auto p = buf;
  i2d_X509(this->cert_, &p);

  const_data_buffer result(buf, len);
  OPENSSL_free(buf);

  return result;
}

void vds::_certificate::load(const filename & filename)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if(nullptr == in){
    auto error = ERR_get_error();
    throw crypto_exception("Failed to load certificate " + filename.str(), error);
  }

  this->cert_ = PEM_read_bio_X509(in, NULL, NULL, NULL);
  if(nullptr == this->cert_){
    auto error = ERR_get_error();
    BIO_free(in);
    throw crypto_exception("Failed to load certificate " + filename.str(), error);
  }
  
  BIO_free(in);
}

void vds::_certificate::save(const filename & filename) const
{
  auto out = BIO_new_file(filename.local_name().c_str(), "w");
  auto ret = PEM_write_bio_X509(out, this->cert_);
  BIO_free_all(out);

  if (1 != ret) {
    auto error = ERR_get_error();
    throw crypto_exception("PEM_write_bio_X509", error);
  }
}

std::string vds::_certificate::subject() const
{
  char result[1024];
  X509_NAME_oneline(X509_get_subject_name(this->cert_), result, sizeof(result));

  return result;
}

std::string vds::_certificate::issuer() const
{
  char result[1024];
  X509_NAME_oneline(X509_get_issuer_name(this->cert_), result, sizeof(result));

  return result;
}

vds::const_data_buffer vds::_certificate::fingerprint(const vds::hash_info & hash_algo) const
{
  unsigned char md[EVP_MAX_MD_SIZE];
  unsigned int n;
  if(!X509_digest(this->cert_, hash_algo.type, md, &n)){
    auto error = ERR_get_error();
    throw crypto_exception("X509_digest", error);
  }
  
  return const_data_buffer(md, n);
}


vds::certificate vds::_certificate::create_new(
  const asymmetric_public_key & new_certificate_public_key,
  const asymmetric_private_key & new_certificate_private_key,
  const certificate::create_options & options
)
{
  X509 * x509 = X509_new();
  if (nullptr == x509) {
    auto error = ERR_get_error();
    throw crypto_exception("X509_new", error);
  }

  try {

    //if (!EVP_PKEY_assign_RSA(new_certificate_private_key.key(), x509)) {
    //  auto error = ERR_get_error();
    //  throw crypto_exception("X509_new", error);
    //}

    X509_set_version(x509, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), (long)60 * 60 * 24 * 365);
    auto key = new_certificate_public_key.impl_->key();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    CRYPTO_add(&key->references,1,CRYPTO_LOCK_EVP_PKEY);
#else
    EVP_PKEY_up_ref(key);
#endif
    X509_set_pubkey(x509, key);

    auto name = X509_get_subject_name(x509);
    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (const unsigned char *)options.country.c_str(), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (const unsigned char *)options.organization.c_str(), -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (const unsigned char *)options.name.c_str(), -1, -1, 0);

    for(auto& extension : options.extensions) {
      add_ext(x509, extension.oid, extension.value.c_str());
    }

    if (nullptr == options.ca_certificate) {
      X509_set_issuer_name(x509, name);
    }
    else {
      X509_set_issuer_name(x509, X509_get_subject_name(options.ca_certificate->impl_->cert()));
    }

    /* Add various extensions: standard extensions */
    add_ext(x509, NID_basic_constraints, "critical,CA:TRUE");
    add_ext(x509, NID_key_usage, "critical,keyCertSign,cRLSign");
    add_ext(x509, NID_subject_key_identifier, "hash");

    /* Some Netscape specific extensions */
    add_ext(x509, NID_netscape_cert_type, "sslCA");
    add_ext(x509, NID_netscape_comment, "example comment extension");

    if (nullptr == options.ca_certificate) {
      if (!X509_sign(x509, new_certificate_private_key.impl_->key(), EVP_sha256())) {
        auto error = ERR_get_error();
        throw crypto_exception("X509_new", error);
      }
    }
    else {
      if (!X509_sign(x509, options.ca_certificate_private_key->impl_->key(), EVP_sha256())) {
        auto error = ERR_get_error();
        throw crypto_exception("X509_new", error);
      }
    }

    return certificate(new _certificate(x509));
  }
  catch (...) {
    X509_free(x509);
    throw;
  }
}

vds::asymmetric_public_key vds::_certificate::public_key() const
{
  auto key = X509_get_pubkey(this->cert_);
  if (nullptr == key) {
    auto error = ERR_get_error();
    throw crypto_exception("X509_get_pubkey", error);
  }
  return asymmetric_public_key(new _asymmetric_public_key(key));
}


bool vds::_certificate::is_ca_cert() const
{
  return (0 < X509_check_ca(this->cert_));
}

bool vds::_certificate::is_issued(const vds::certificate& issuer) const
{
  return (X509_V_OK == X509_check_issued(issuer.impl_->cert(), this->cert()));
}

bool vds::_certificate::add_ext(X509 * cert, int nid, const char * value)
{
  X509_EXTENSION * ex = X509V3_EXT_conf_nid(NULL, NULL, nid, const_cast<char *>(value));
  if (nullptr == ex) {
    return false;
  }

  X509_add_ext(cert, ex, -1);
  X509_EXTENSION_free(ex);

  return true;
}

int vds::_certificate::extension_count() const
{
  return X509_get_ext_count(this->cert_);
}

int vds::_certificate::extension_by_NID(int nid) const
{
  return X509_get_ext_by_NID(this->cert_, nid, -1);
}

vds::certificate_extension vds::_certificate::get_extension(int index) const
{
  certificate_extension result;
  
  X509_EXTENSION * ext = X509_get_ext(this->cert_, index);
  if(nullptr != ext){
    auto obj = X509_EXTENSION_get_object(ext);
    result.oid = OBJ_obj2nid(obj);
    
    //char buf[256];
    //OBJ_obj2txt(buf, sizeof(buf), obj, 0);
    //result.name = buf;
    
    BIO *bio = BIO_new(BIO_s_mem());
    if(!X509V3_EXT_print(bio, ext, 0, 0)){
      //M_ASN1_OCTET_STRING_print(bio, ext);
      throw std::runtime_error("Unable get certificate extension");
    }
    BIO_flush(bio);
    
    char buf[256];
    auto len = BIO_read(bio, buf, sizeof(buf));
    BIO_free(bio);
    
    result.value.assign(buf, len);
  }
  
  return result;
}

//////////////////////////////////////////////////////////
vds::certificate_store::certificate_store()
: impl_(new _certificate_store())
{
}

vds::certificate_store::~certificate_store()
{
  delete this->impl_;
}

void vds::certificate_store::add(const vds::certificate& cert)
{
  this->impl_->add(cert);
}


void vds::certificate_store::load_locations(const std::string & location)
{
  this->impl_->load_locations(location);
}

vds::certificate_store::verify_result vds::certificate_store::verify(const vds::certificate& cert) const
{
  return this->impl_->verify(cert);
}

//////////////////////////////////////////////////////////
vds::_certificate_store::_certificate_store()
: store_(X509_STORE_new())
{
  if (nullptr == this->store_) {
    auto error = ERR_get_error();
    throw crypto_exception("X509_get_pubkey", error);
  }
}

vds::_certificate_store::~_certificate_store()
{
  if (nullptr != this->store_) {
    X509_STORE_free(this->store_);
  }
}

void vds::_certificate_store::add(const vds::certificate& cert)
{
  if(0 >= X509_STORE_add_cert(this->store_, cert.impl_->cert())){
    auto error = ERR_get_error();
    throw crypto_exception("unable to add certificate to store", error);
  }
}


void vds::_certificate_store::load_locations(const std::string & location)
{
  if(0 >= X509_STORE_load_locations(this->store_, location.c_str(), NULL)){
    auto error = ERR_get_error();
    throw crypto_exception("unable to load certificates at " + location + " to store", error);
  }
}

vds::certificate_store::verify_result vds::_certificate_store::verify(const vds::certificate& cert) const
{
  X509_STORE_CTX * vrfy_ctx = X509_STORE_CTX_new();
  if (nullptr == vrfy_ctx) {
    auto error = ERR_get_error();
    throw crypto_exception("X509_STORE_CTX_new", error);
  }

  try {

    X509_STORE_CTX_init(vrfy_ctx, this->store_, cert.impl_->cert(), NULL);

    certificate_store::verify_result result;
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


