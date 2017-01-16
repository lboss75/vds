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

void vds::hash::update(const void * data, int len)
{
  if (1 != EVP_DigestUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw new crypto_exception("EVP_DigestUpdate", error);
  }
}

void vds::hash::final()
{
  this->sig_len_ = EVP_MD_size(this->info_.type);
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
