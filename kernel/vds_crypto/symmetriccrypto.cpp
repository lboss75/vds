/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "symmetriccrypto.h"

vds::symmetric_crypto_info::symmetric_crypto_info(const EVP_CIPHER* cipher)
: cipher_(cipher)
{
}

size_t vds::symmetric_crypto_info::key_size() const
{
  return EVP_CIPHER_key_length(this->cipher_);
}

size_t vds::symmetric_crypto_info::iv_size() const
{
  return EVP_CIPHER_iv_length(this->cipher_);
}

const vds::symmetric_crypto_info& vds::symmetric_crypto::aes_256_cbc()
{
  static symmetric_crypto_info result(EVP_aes_256_cbc());
  
  return result;
}

vds::symmetric_key::symmetric_key(const vds::symmetric_crypto_info& crypto_info)
: crypto_info_(crypto_info)
{
}

void vds::symmetric_key::generate()
{
  this->key_.reset(new unsigned char[this->crypto_info_.key_size()]);
  this->iv_.reset(new unsigned char[this->crypto_info_.iv_size()]);
  
  RAND_bytes(this->key_.get(), (int)this->crypto_info_.key_size());
  RAND_bytes(this->iv_.get(), (int)this->crypto_info_.iv_size());
}

vds::symmetric_encrypt::symmetric_encrypt(const vds::symmetric_key& key)
: ctx_(EVP_CIPHER_CTX_new())
{
  if(nullptr == this->ctx_){
    throw new std::runtime_error("Create crypto context failed");
  }
  
  if(1 != EVP_EncryptInit_ex(
    this->ctx_,
    key.crypto_info_.cipher(),
    nullptr,
    key.key(),
    key.iv())) {
    throw new std::runtime_error("Create crypto context failed");
  }
}

vds::symmetric_encrypt::~symmetric_encrypt()
{
  if(nullptr != this->ctx_){
    EVP_CIPHER_CTX_free(this->ctx_);
  }
}

size_t vds::symmetric_encrypt::update(
  const void* data, size_t len,
  void* result_data, size_t result_data_len)
{
  int result_len = (int)result_data_len;
  if(0 == len)
  {
    if(1 != EVP_EncryptFinal_ex(this->ctx_,
      (unsigned char *)result_data, &result_len)) {
      throw new std::runtime_error("Crypt failed");
    }
  }
  else if(1 != EVP_EncryptUpdate(this->ctx_,
    (unsigned char *)result_data, &result_len,
    (const unsigned char *)data, (int)len)){
    throw new std::runtime_error("Crypt failed");
  }
  
  return (size_t)result_len;
}

vds::symmetric_decrypt::symmetric_decrypt(const vds::symmetric_key& key)
: ctx_(EVP_CIPHER_CTX_new())
{
  if(nullptr == this->ctx_){
    throw new std::runtime_error("Create crypto context failed");
  }
  
  if(1 != EVP_DecryptInit_ex(
    this->ctx_,
    key.crypto_info_.cipher(),
    nullptr,
    key.key(),
    key.iv())) {
    throw new std::runtime_error("Create crypto context failed");
  }
}

vds::symmetric_decrypt::~symmetric_decrypt()
{
  if(nullptr != this->ctx_){
    EVP_CIPHER_CTX_free(this->ctx_);
  }
}

size_t vds::symmetric_decrypt::update(const void* data, size_t len, void* result_data, size_t result_data_len)
{
  int result_len = (int)result_data_len;
  if(0 == len)
  {
    if(1 != EVP_DecryptFinal_ex(this->ctx_,
      (unsigned char *)result_data, &result_len)) {
      throw new std::runtime_error("Decrypt failed");
    }
  }
  else if(1 != EVP_DecryptUpdate(this->ctx_,
    (unsigned char *)result_data, &result_len,
    (const unsigned char *)data, (int)len)){
    throw new std::runtime_error("Decrypt failed");
  }
  
  return (size_t)result_len;
}


