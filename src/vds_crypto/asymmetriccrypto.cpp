/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "asymmetriccrypto.h"
#include "crypto_exception.h"

vds::asymmetric_private_key::asymmetric_private_key()
  : info_(vds::asymmetric_crypto::unknown()),
  key_(nullptr),
  ctx_(nullptr)
{
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
  EVP_PKEY_CTX_free(this->ctx_);
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

void vds::asymmetric_private_key::load(const filename & filename)
{
  auto in = BIO_new_file(filename.local_name().c_str(), "r");
  if (nullptr == in) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to private key " + filename.str(), error);
  }

  RSA * r = PEM_read_bio_RSAPrivateKey(in, NULL, NULL, NULL);
  if (nullptr == r) {
    auto error = ERR_get_error();
    throw new crypto_exception("Failed to private key " + filename.str(), error);
  }

  this->key_ = EVP_PKEY_new();
  EVP_PKEY_assign_RSA(this->key_, r);

  BIO_free(in);
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

vds::certificate::certificate()
: cert_(nullptr)
{
}

vds::certificate::~certificate()
{
  if(nullptr != this->cert_){
    X509_free(this->cert_);
  }
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
