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

const vds::hash_info & vds::hash::sha1()
{
	static hash_info result = {
	  NID_sha1,
	  EVP_sha1()
	};

	return result;
}
///////////////////////////////////////////////////////////////
vds::hash::hash()
: impl_(nullptr)
{
}

vds::hash::hash(hash&& original) noexcept
: impl_(original.impl_){
  original.impl_ = nullptr;
}

vds::hash::~hash()
{
  delete this->impl_;
}

vds::expected<vds::hash> vds::hash::create(const hash_info & info)
{
  auto impl = std::make_unique<_hash>();
  CHECK_EXPECTED(impl->create(info));
  return hash(impl.release());
}

vds::expected<void> vds::hash::update(const void * data, size_t len)
{
  return this->impl_->update(data, len);
}

vds::expected<void> vds::hash::final()
{
  return this->impl_->final();
}

const vds::const_data_buffer& vds::hash::signature() const
{
  return this->impl_->signature();

}

vds::expected<vds::const_data_buffer> vds::hash::signature(const hash_info& info, expected<const_data_buffer>&& data) {
  CHECK_EXPECTED_ERROR(data);
  return signature(info, data.value());
}

vds::expected<vds::const_data_buffer> vds::hash::signature(
  const vds::hash_info& info,
  const const_data_buffer& data)
{
  return signature(info, data.data(), data.size());
}

vds::expected<vds::const_data_buffer> vds::hash::signature(
  const vds::hash_info& info,
  const void * data,
  size_t data_size)
{
  GET_EXPECTED(h, hash::create(info));
  CHECK_EXPECTED(h.update(data, data_size));
  CHECK_EXPECTED(h.final());
  
  return h.signature();
}

vds::hash& vds::hash::operator=(hash&& original) noexcept {
  delete this->impl_;
  this->impl_ = original.impl_;
  original.impl_ = nullptr;
  return *this;
}

vds::hash_stream_output_async::hash_stream_output_async() {
}

vds::hash_stream_output_async::hash_stream_output_async(hash&& hash,
  std::shared_ptr<stream_output_async<uint8_t>>&& target)
: hash_(std::move(hash)), target_(std::move(target)){
}

vds::expected<std::shared_ptr<vds::hash_stream_output_async>> vds::hash_stream_output_async::create(
  const hash_info& info,
  std::shared_ptr<stream_output_async<uint8_t>> && target) {
  GET_EXPECTED(h, hash::create(info));
  return std::make_shared<hash_stream_output_async>(std::move(h), std::move(target));
}


vds::async_task<vds::expected<void>> vds::hash_stream_output_async::write_async(const uint8_t* data, size_t len) {
  if(len != 0) {
    CHECK_EXPECTED_ASYNC(this->hash_.update(data, len));
    CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(data, len));
  }
  else {
    CHECK_EXPECTED_ASYNC(this->hash_.final());
    CHECK_EXPECTED_ASYNC(co_await this->target_->write_async(data, len));
  }
  co_return expected<void>();
}

///////////////////////////////////////////////////////////////
vds::_hash::_hash()
  : info_(nullptr), ctx_(nullptr)
{
}

vds::_hash::~_hash()
{
  if (nullptr != this->ctx_) {
    EVP_MD_CTX_destroy(this->ctx_);
  }
}

vds::expected<void> vds::_hash::create(const hash_info & info)
{
  this->info_ = &info;
    this->ctx_ = EVP_MD_CTX_create();

  if (nullptr == this->ctx_) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_MD_CTX_create", error);
  }

  if (1 != EVP_DigestInit_ex(this->ctx_, info.type, NULL)) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_DigestInit_ex", error);
  }

  return expected<void>();
}

vds::expected<void> vds::_hash::update(const void * data, size_t len)
{
  if (1 != EVP_DigestUpdate(this->ctx_, data, len)) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_DigestUpdate", error);
  }

  return expected<void>();
}

vds::expected<void> vds::_hash::final()
{
  auto len = (unsigned int)EVP_MD_size(this->info_->type);
  this->sig_.resize(len);

  if (1 != EVP_DigestFinal_ex(this->ctx_, this->sig_.data(), &len)) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_DigestFinal_ex", error);
  }

  if (len != this->sig_.size()) {
    return vds::make_unexpected<std::runtime_error>("len != this->sig_len_");
  }

  return expected<void>();
}
///////////////////////////////////////////////////////////////
vds::hmac::hmac(const const_data_buffer & key, const hash_info & info)
: impl_(new _hmac(key, info))
{
}

vds::hmac::~hmac()
{
  delete this->impl_;
}

vds::expected<void> vds::hmac::update(const void * data, size_t len)
{
  return this->impl_->update(data, len);
}

vds::expected<vds::const_data_buffer> vds::hmac::final()
{
  return this->impl_->final();
}

///////////////////////////////////////////////////////////////

vds::_hmac::_hmac(
    const const_data_buffer & key,
    const hash_info & info)
: info_(info)
{
#if OPENSSL_VERSION_NUMBER < 0x1010007fL
  this->ctx_ = &this->ctx_data_;
  HMAC_CTX_init(this->ctx_);
#else
  this->ctx_ = HMAC_CTX_new();
#endif

  HMAC_Init_ex(this->ctx_, key.data(), safe_cast<int>(key.size()), info.type, NULL);
}

vds::_hmac::~_hmac()
{
#if OPENSSL_VERSION_NUMBER < 0x1010007fL
  HMAC_CTX_cleanup(this->ctx_);
#else
  HMAC_CTX_free(this->ctx_);
#endif//_WIN32
}

vds::expected<void> vds::_hmac::update(const void * data, size_t len) {
  if (1 != HMAC_Update(this->ctx_, reinterpret_cast<const unsigned char *>(data), len)) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("EVP_DigestUpdate", error);
  }

  return expected<void>();
}

vds::expected<vds::const_data_buffer> vds::_hmac::final() {

  auto result_len = (unsigned int)EVP_MD_size(this->info_.type);
  const_data_buffer result;
  result.resize(result_len);
  if (1 != HMAC_Final(this->ctx_, result.data(), &result_len)) {
    auto error = ERR_get_error();
    return vds::make_unexpected<crypto_exception>("HMAC_Final", error);
  }

  if (result_len != result.size()) {
    return vds::make_unexpected<std::runtime_error>("len != this->sig_len_");
  }

  return result;
}
