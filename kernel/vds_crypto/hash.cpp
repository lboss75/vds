/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "hash.h"
#include "crypto_exception.h"

const vds::hash_info & vds::hash::sha256()
{
  static hash_info result = {
    NID_sha256,
    EVP_sha256()
  };

  return result;
}

vds::hash::hash(const hash_info & info)
  : info_(info), sig_(nullptr)
{
  this->ctx_ = EVP_MD_CTX_create();

  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_MD_CTX_create", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, info.type, NULL)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestInit_ex", error);
  }

}

vds::hash::~hash()
{
  if (nullptr != this->sig_) {
    OPENSSL_free(this->sig_);
  }

  if (nullptr != this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::hash::update(const void * data, size_t len)
{
  if (1 != EVP_DigestUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestUpdate", error);
  }
}

void vds::hash::final()
{
  this->sig_len_ = (unsigned int)EVP_MD_size(this->info_.type);
  this->sig_ = (unsigned char *)OPENSSL_malloc(this->sig_len_);
  if (nullptr == this->sig_) {
    throw new std::runtime_error("out of memory");
  }

  auto len = this->sig_len_;
  if (1 != EVP_DigestFinal_ex(this->ctx_, this->sig_, &len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestFinal_ex", error);
  }

  if (len != this->sig_len_) {
    throw new std::runtime_error("len != this->sig_len_");
  }
}

vds::hmac::hmac(const std::string & key, const hash_info & info)
: info_(info)
{
  this->ctx_ = HMAC_CTX_new();

  HMAC_Init_ex(this->ctx_, key.c_str(), key.length(), info.type, NULL);
}

vds::hmac::~hmac()
{
  HMAC_CTX_free(this->ctx_);
}

void vds::hmac::update(const void * data, size_t len)
{
  if (1 != HMAC_Update(this->ctx_, reinterpret_cast<const unsigned char*>(data), len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestUpdate", error);
  }
}

void vds::hmac::final()
{
  this->sig_len_ = (unsigned int)EVP_MD_size(this->info_.type);
  this->sig_ = (unsigned char *)OPENSSL_malloc(this->sig_len_);
  if (nullptr == this->sig_) {
    throw new std::runtime_error("out of memory");
  }

  auto len = this->sig_len_;
  if (1 != HMAC_Final(this->ctx_, this->sig_, &len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("HMAC_Final", error);
  }

  if (len != this->sig_len_) {
    throw new std::runtime_error("len != this->sig_len_");
  }
}
