/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "symmetriccrypto.h"
#include "private/symmetriccrypto_p.h"

////////////////////////////////////////////////////////////////////////////
vds::symmetric_crypto_info::symmetric_crypto_info(_symmetric_crypto_info * impl)
: impl_(impl)
{
}

size_t vds::symmetric_crypto_info::key_size() const
{
  return this->impl_->key_size();
}

size_t vds::symmetric_crypto_info::iv_size() const
{
  return this->impl_->iv_size();
}

size_t vds::symmetric_crypto_info::block_size() const
{
  return this->impl_->block_size();
}

const vds::symmetric_crypto_info& vds::symmetric_crypto::aes_256_cbc()
{
  static _symmetric_crypto_info _result(EVP_aes_256_cbc());
  static symmetric_crypto_info result(&_result);
  
  return result;
}

const vds::symmetric_crypto_info& vds::symmetric_crypto::rc4()
{
  static _symmetric_crypto_info _result(EVP_rc4());
  static symmetric_crypto_info result(&_result);
  
  return result;
}

vds::symmetric_key vds::symmetric_key::generate(const symmetric_crypto_info & crypto_info)
{
  auto key = new unsigned char[crypto_info.key_size()];
  auto iv = new unsigned char[crypto_info.iv_size()];
  
  RAND_bytes(key, (int)crypto_info.key_size());
  RAND_bytes(iv, (int)crypto_info.iv_size());

  return symmetric_key(new _symmetric_key(crypto_info, key, iv));
}

size_t vds::symmetric_key::block_size() const
{
  return this->impl_->crypto_info_.block_size();
}

vds::expected<vds::symmetric_key> vds::symmetric_key::deserialize(
    const vds::symmetric_crypto_info& crypto_info,
    vds::binary_deserializer& s)
{
  std::unique_ptr<unsigned char> key(new unsigned char[crypto_info.key_size()]);
  CHECK_EXPECTED(s.pop_data(key.get(), (int)crypto_info.key_size()));

  std::unique_ptr<unsigned char>  iv(new unsigned char[crypto_info.iv_size()]);
  CHECK_EXPECTED(s.pop_data(iv.get(), (int)crypto_info.iv_size()));

  return symmetric_key(new _symmetric_key(crypto_info, key.release(), iv.release()));
}

vds::expected<vds::symmetric_key> vds::symmetric_key::deserialize(
    const vds::symmetric_crypto_info& crypto_info,
    vds::binary_deserializer && s)
{
  std::unique_ptr<unsigned char> key(new unsigned char[crypto_info.key_size()]);
  GET_EXPECTED(key_size, s.pop_data(key.get(), (int)crypto_info.key_size()));
  if (crypto_info.key_size() != key_size) {
    return vds::make_unexpected<std::runtime_error>("Invalid data");
  }

  std::unique_ptr<unsigned char> iv(new unsigned char[crypto_info.iv_size()]);
  GET_EXPECTED(iv_size, s.pop_data(iv.get(), (int)crypto_info.iv_size()));
  if(crypto_info.iv_size() != iv_size){
    return vds::make_unexpected<std::runtime_error>("Invalid data");
  }

  return symmetric_key(new _symmetric_key(crypto_info, key.release(), iv.release()));
}

vds::expected<void> vds::symmetric_key::serialize(vds::binary_serializer& s) const
{
  CHECK_EXPECTED(s.push_data(this->impl_->key_, (int)this->impl_->crypto_info_.key_size()));
  return s.push_data(this->impl_->iv_, (int)this->impl_->crypto_info_.iv_size());
}

vds::expected<vds::symmetric_key> vds::symmetric_key::from_password(const std::string & password)
{
  std::unique_ptr<unsigned char> key(new unsigned char[EVP_MAX_KEY_LENGTH]);
  if(!EVP_BytesToKey(EVP_rc4(), EVP_md5(), NULL, (const unsigned char *)password.c_str(), password.length(), 1, key.get(), NULL)){
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_BytesToKey failed", error);
  }
  
  return symmetric_key(new _symmetric_key(symmetric_crypto::rc4(), key.release(), nullptr));
}

vds::symmetric_key::symmetric_key()
: impl_(nullptr) {
}

vds::symmetric_key::symmetric_key(_symmetric_key *impl)
: impl_(impl){
}

vds::symmetric_key::symmetric_key(symmetric_key&& origin)
: impl_(origin.impl_){
  origin.impl_ = nullptr;
}

vds::symmetric_key::~symmetric_key() {
}

vds::symmetric_key vds::symmetric_key::create(
    const symmetric_crypto_info &crypto_info,
    const uint8_t *key,
    const uint8_t *iv) {
  std::unique_ptr<unsigned char> key_(new unsigned char[crypto_info.key_size()]);
  std::unique_ptr<unsigned char> iv_(new unsigned char[crypto_info.iv_size()]);

  memcpy(key_.get(), key, crypto_info.key_size());
  memcpy(iv_.get(), iv, crypto_info.iv_size());

  return symmetric_key(new _symmetric_key(crypto_info, key_.release(), iv_.release()));
}

//////////////////////////////////////////////////////////////
vds::_symmetric_key::_symmetric_key(
    const symmetric_crypto_info &crypto_info,
    uint8_t *key, uint8_t *iv)
: crypto_info_(crypto_info), key_(key), iv_(iv) {

}

vds::_symmetric_key::~_symmetric_key()
{
	delete[] this->key_;
	delete[] this->iv_;
}
///////////////////////////////////////////////////
vds::symmetric_encrypt::symmetric_encrypt()
: impl_(nullptr)
{
}

vds::symmetric_encrypt::~symmetric_encrypt() {
  delete this->impl_;
}

vds::expected<std::shared_ptr<vds::symmetric_encrypt>> vds::symmetric_encrypt::create(
  const vds::symmetric_key& key,
  const std::shared_ptr<stream_output_async<uint8_t>> & target) {

  auto impl = new _symmetric_encrypt();
  CHECK_EXPECTED(impl->create(key, target));

  return std::make_shared<symmetric_encrypt>(impl);
}


vds::async_task<vds::expected<void>> vds::symmetric_encrypt::write_async(const uint8_t* data, size_t len) {
  return this->impl_->write_async(data, len);
}

vds::expected<vds::const_data_buffer> vds::symmetric_encrypt::encrypt(
  const vds::symmetric_key& key,
  const void * input_buffer,
  size_t input_buffer_size) {
  thread_unprotect thread_unprotect;

  auto result = std::make_shared<collect_data>();

  _symmetric_encrypt s;
  CHECK_EXPECTED(s.create(key, result));
  CHECK_EXPECTED(s.write_async((const uint8_t *)input_buffer, input_buffer_size).get());
  CHECK_EXPECTED(s.write_async(nullptr, 0).get());

  return result->move_data();
}


////////////////////////////////////////////////////////////////////////////
vds::_symmetric_crypto_info::_symmetric_crypto_info(const EVP_CIPHER* cipher)
: cipher_(cipher)
{
}

size_t vds::_symmetric_crypto_info::key_size() const
{
  return EVP_CIPHER_key_length(this->cipher_);
}

size_t vds::_symmetric_crypto_info::iv_size() const
{
  return EVP_CIPHER_iv_length(this->cipher_);
}

vds::symmetric_decrypt::symmetric_decrypt()
  : impl_(nullptr) {
}

vds::symmetric_decrypt::~symmetric_decrypt() {
  delete this->impl_; 
}

vds::expected<void> vds::symmetric_decrypt::create(
  const symmetric_key & key,
  const std::shared_ptr<stream_output_async<uint8_t>> & target) {
  this->impl_ = new _symmetric_decrypt();
  return this->impl_->create(key, target);
}

vds::expected<vds::const_data_buffer> vds::symmetric_decrypt::decrypt(
  const symmetric_key & key,
  const void * input_buffer,
  size_t input_buffer_size)
{
  thread_unprotect thread_unprotect;

  auto result = std::make_shared<collect_data>();
   
  _symmetric_decrypt s;
  CHECK_EXPECTED(s.create(key, result));
  CHECK_EXPECTED(s.write_async((const uint8_t *)input_buffer, input_buffer_size).get());
  CHECK_EXPECTED(s.write_async(nullptr, 0).get());

  return result->move_data();
}
