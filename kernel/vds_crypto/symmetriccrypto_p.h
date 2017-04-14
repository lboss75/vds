#ifndef __VDS_CRYPTO_SYMMETRICCRYPTO_P_H_
#define __VDS_CRYPTO_SYMMETRICCRYPTO_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "symmetriccrypto.h"
#include "crypto_exception.h"

namespace vds {
  class symmetric_encrypt;
  class symmetric_decrypt;
    
  class _symmetric_crypto_info
  {
  public:
    _symmetric_crypto_info(const EVP_CIPHER * cipher);
    
    const EVP_CIPHER * cipher() const {
      return this->cipher_;
    }
    
    size_t key_size() const;
    size_t iv_size() const;

    size_t block_size() const
    {
      return EVP_CIPHER_block_size(this->cipher_);
    }

  private:
    const EVP_CIPHER * cipher_;
  };
  
  class _symmetric_crypto
  {
  public:
    static const symmetric_crypto_info & aes_256_cbc();
  };
  
  class _symmetric_key
  {
  public:
    _symmetric_key(const symmetric_crypto_info & crypto_info);
    
    void generate();
    
    const unsigned char * key() const {
      return this->key_.get();
    }
    
    const unsigned char * iv() const {
      return this->iv_.get();
    }    
    
  private:
    friend class symmetric_encrypt;
    friend class symmetric_decrypt;
    
    const symmetric_crypto_info & crypto_info_;
    std::unique_ptr<unsigned char> key_;
    std::unique_ptr<unsigned char> iv_;
  };
  
  class _symmetric_encrypt
  {
  public:
    _symmetric_encrypt(const symmetric_key & key)
    : ctx_(EVP_CIPHER_CTX_new()),
      len_(0),
      eof_(false),
      block_size_(key.block_size())
    {
      if (nullptr == this->ctx_) {
        throw new std::runtime_error("Create crypto context failed");
      }

      if (1 != EVP_EncryptInit_ex(
        this->ctx_,
        key.crypto_info_.impl_->cipher(),
        nullptr,
        key.key(),
        key.iv())) {
        throw new std::runtime_error("Create crypto context failed");
      }
    }

    ~_symmetric_encrypt()
    {
      if (nullptr != this->ctx_) {
        EVP_CIPHER_CTX_free(this->ctx_);
      }
    }
    
    bool update(
      const void * data,
      size_t len,
      void * result_data,
      size_t & result_data_len)
    {
      assert(0 == this->len_);

      if (0 == len) {
        this->eof_ = true;

        int buf_len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
        if (0 == EVP_CipherFinal_ex(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &buf_len)) {
          auto error = ERR_get_error();
          throw new crypto_exception("EVP_CipherFinal_ex failed", error);
        }

        result_data_len = buf_len;

        return true;
      }

      this->data_ = reinterpret_cast<const uint8_t *>(data);
      this->len_ = len;

      return this->processed(result_data, result_data_len);
    }

    bool processed(
      void * result_data,
      size_t & result_data_len)
    {
      if (this->eof_) {
        result_data_len = 0;
        return true;
      }

      if (0 == this->len_) {
        return false;
      }


      int len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
      auto n = this->len_;
      if (n > len - this->block_size_ + 1) {
        n = len - this->block_size_ + 1;
      }

      if (0 == EVP_CipherUpdate(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &len, this->data_, (int)n)) {
        auto error = ERR_get_error();
        throw new crypto_exception("EVP_CipherUpdate failed", error);
      }

      this->data_ += n;
      this->len_ -= n;

      result_data_len = len;
      return 0 != result_data_len;
    }

  private:
    EVP_CIPHER_CTX * ctx_;
    bool eof_;
    const uint8_t * data_;
    size_t len_;
    size_t block_size_;
  };
  
  class _symmetric_decrypt
  {
  public:
    _symmetric_decrypt(const symmetric_key & key)
      : ctx_(EVP_CIPHER_CTX_new()),
      len_(0),
      eof_(false),
      block_size_(key.block_size()) {
      if (nullptr == this->ctx_) {
        throw new std::runtime_error("Create crypto context failed");
      }

      if (1 != EVP_DecryptInit_ex(
        this->ctx_,
        key.crypto_info_.impl_->cipher(),
        nullptr,
        key.key(),
        key.iv())) {
        throw new std::runtime_error("Create decrypt context failed");
      }
    }

    ~_symmetric_decrypt()
    {
      if (nullptr != this->ctx_) {
        EVP_CIPHER_CTX_free(this->ctx_);
      }
    }

    bool update(
      const void * data,
      size_t len,
      void * result_data,
      size_t & result_data_len)
    {
      assert(0 == this->len_);

      if (0 == len) {
        this->eof_ = true;

        int buf_len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
        if (0 == EVP_CipherFinal_ex(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &buf_len)) {
          auto error = ERR_get_error();
          throw new crypto_exception("EVP_CipherFinal_ex failed", error);
        }

        result_data_len = buf_len;

        return true;
      }

      this->data_ = reinterpret_cast<const uint8_t *>(data);
      this->len_ = len;

      return this->processed(result_data, result_data_len);
    }

    bool processed(
      void * result_data,
      size_t & result_data_len)
    {
      if (this->eof_) {
        result_data_len = 0;
        return true;
      }

      if (0 == this->len_) {
        return false;
      }

      int len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
      auto n = this->len_;
      if (n > len - this->block_size_ + 1) {
        n = len - this->block_size_ + 1;
      }
      if (0 == EVP_CipherUpdate(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &len, this->data_, (int)n)) {
        auto error = ERR_get_error();
        throw new crypto_exception("EVP_CipherUpdate failed", error);
      }

      this->data_ += n;
      this->len_ -= n;

      result_data_len = len;
      return 0 != result_data_len;
    }

  private:
    EVP_CIPHER_CTX * ctx_;
    bool eof_;
    const uint8_t * data_;
    size_t len_;
    size_t block_size_;
  };
  
}

#endif // __VDS_CRYPTO_SYMMETRICCRYPTO_P_H_
