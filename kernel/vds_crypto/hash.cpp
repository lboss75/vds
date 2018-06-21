/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "hash.h"
#include "private/hash_p.h"
#include "crypto_exception.h"

const vds::hash_info & vds::hash::md5()
{
  static hash_info result = {
    NID_md5,
    EVP_md5()
  };

  return result;
}

const vds::hash_info & vds::hash::sha256()
{
  static hash_info result = {
    NID_sha256,
    EVP_sha256()
  };

  return result;
}
///////////////////////////////////////////////////////////////
vds::hash::hash(const hash_info & info)
: impl_(new _hash(info))
{
}

vds::hash::~hash()
{
  delete this->impl_;
}

void vds::hash::update(const void * data, size_t len)
{
  this->impl_->update(data, len);
}

void vds::hash::final()
{
  this->impl_->final();
}

const vds::const_data_buffer& vds::hash::signature() const
{
  return this->impl_->signature();

}

vds::const_data_buffer vds::hash::signature(
  const vds::hash_info& info,
  const const_data_buffer& data)
{
  return signature(info, data.data(), data.size());
}

vds::const_data_buffer vds::hash::signature(
  const vds::hash_info& info,
  const void * data,
  size_t data_size)
{
  hash h(info);
  h.update(data, data_size);
  h.final();
  
  return h.signature();
}
///////////////////////////////////////////////////////////////
vds::_hash::_hash(const hash_info & info)
  : info_(info)
{
  this->ctx_ = EVP_MD_CTX_create();

  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_MD_CTX_create", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, info.type, NULL)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestInit_ex", error);
  }

}

vds::_hash::~_hash()
{
  if (nullptr != this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

void vds::_hash::update(const void * data, size_t len)
{
  if (1 != EVP_DigestUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestUpdate", error);
  }
}

void vds::_hash::final()
{
  auto len = (unsigned int)EVP_MD_size(this->info_.type);
  std::vector<unsigned char> buffer(len);

  if (1 != EVP_DigestFinal_ex(this->ctx_, buffer.data(), &len)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestFinal_ex", error);
  }

  if (len != buffer.size()) {
    throw std::runtime_error("len != this->sig_len_");
  }
  
  this->sig_.reset(buffer.data(), buffer.size());  
}
///////////////////////////////////////////////////////////////
vds::hmac::hmac(const std::string & key, const hash_info & info)
: impl_(new _hmac(key, info))
{
}

vds::hmac::~hmac()
{
  delete this->impl_;
}

void vds::hmac::update(const void * data, size_t len)
{
  this->impl_->update(data, len);
}

void vds::hmac::final()
{
  this->impl_->final();
}
///////////////////////////////////////////////////////////////

vds::_hmac::_hmac(const std::string & key, const hash_info & info)
: info_(info)
{
#ifndef _WIN32
  this->ctx_ = &this->ctx_data_;
#else
  this->ctx_ = HMAC_CTX_new();
#endif

  HMAC_Init_ex(this->ctx_, key.c_str(), safe_cast<int>(key.length()), info.type, NULL);
}

vds::_hmac::~_hmac()
{
#ifdef _WIN32
  HMAC_CTX_free(this->ctx_);
#else
  HMAC_CTX_cleanup(this->ctx_);
#endif//_WIN32
}

void vds::_hmac::update(const void * data, size_t len)
{
  if (1 != HMAC_Update(this->ctx_, reinterpret_cast<const unsigned char*>(data), len)) {
    auto error = ERR_get_error();
    throw crypto_exception("EVP_DigestUpdate", error);
  }
}

void vds::_hmac::final()
{
  auto len = (unsigned int)EVP_MD_size(this->info_.type);
  std::vector<unsigned char> buffer(len);
  if (1 != HMAC_Final(this->ctx_, buffer.data(), &len)) {
    auto error = ERR_get_error();
    throw crypto_exception("HMAC_Final", error);
  }

  if (len != buffer.size()) {
    throw std::runtime_error("len != this->sig_len_");
  }
  
  this->sig_.reset(buffer.data(), buffer.size());
}
