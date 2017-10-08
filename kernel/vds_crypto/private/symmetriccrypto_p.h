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
      eof_(false),
      block_size_(key.block_size())
    {
      if (nullptr == this->ctx_) {
        throw std::runtime_error("Create crypto context failed");
      }

      if (1 != EVP_EncryptInit_ex(
        this->ctx_,
        key.crypto_info_.impl_->cipher(),
        nullptr,
        key.key(),
        key.iv())) {
        throw std::runtime_error("Create crypto context failed");
      }
    }

    ~_symmetric_encrypt()
    {
      if (nullptr != this->ctx_) {
        EVP_CIPHER_CTX_free(this->ctx_);
      }
    }
    
    void update(
      const void * input_buffer,
      size_t input_buffer_size,
      void * output_buffer,
      size_t output_buffer_size,
      size_t & input_readed,
      size_t & output_written)
    {
      if (0 < input_buffer_size) {
        int len = (output_buffer_size > INT_MAX) ? INT_MAX : (int)output_buffer_size;
        auto n = input_buffer_size;
        if (n > len - this->block_size_ + 1) {
          n = len - this->block_size_ + 1;
        }

        if (0 == EVP_CipherUpdate(this->ctx_, reinterpret_cast<unsigned char *>(output_buffer), &len, input_buffer, (int)n)) {
          auto error = ERR_get_error();
          throw crypto_exception("EVP_CipherUpdate failed", error);
        }

        input_readed = n;
        output_written = len;
      }
      else if (!this->eof_) {
        this->eof_ = true;

        int buf_len = (output_buffer_size > INT_MAX) ? INT_MAX : (int)output_buffer_size;
        if (0 == EVP_CipherFinal_ex(this->ctx_, reinterpret_cast<unsigned char *>(output_buffer), &buf_len)) {
          auto error = ERR_get_error();
          throw crypto_exception("EVP_CipherFinal_ex failed", error);
        }

        input_readed = 0;
        output_written = buf_len;
      }
      else {
        input_readed = 0;
        output_written = 0;
      }
    }


  private:
    EVP_CIPHER_CTX * ctx_;
    bool eof_;
    size_t block_size_;
  };
  
  class _symmetric_decrypt
  {
  public:
    _symmetric_decrypt(const symmetric_key & key)
      : ctx_(EVP_CIPHER_CTX_new()),
      eof_(false),
      block_size_(key.block_size()) {
      if (nullptr == this->ctx_) {
        throw std::runtime_error("Create crypto context failed");
      }

      if (1 != EVP_DecryptInit_ex(
        this->ctx_,
        key.crypto_info_.impl_->cipher(),
        nullptr,
        key.key(),
        key.iv())) {
        throw std::runtime_error("Create decrypt context failed");
      }
    }

    ~_symmetric_decrypt()
    {
      if (nullptr != this->ctx_) {
        EVP_CIPHER_CTX_free(this->ctx_);
      }
    }

    void update(
      const uint8_t * input_data,
      size_t input_data_len,
      uint8_t * result_data,
      size_t result_data_len,
      size_t & input_readed,
      size_t & output_written)
    {
      if (0 < input_data_len) {
        int len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
        auto n = input_data_len;
        if (n > len - this->block_size_ + 1) {
          n = len - this->block_size_ + 1;
        }
        if (0 == EVP_CipherUpdate(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &len, input_data, (int)n)) {
          auto error = ERR_get_error();
          throw crypto_exception("EVP_CipherUpdate failed", error);
        }

        input_readed = n;
        output_written = len;
      }
      else if(!this->eof_) {
        this->eof_ = true;

        int buf_len = (result_data_len > INT_MAX) ? INT_MAX : (int)result_data_len;
        if (0 == EVP_CipherFinal_ex(this->ctx_, reinterpret_cast<unsigned char *>(result_data), &buf_len)) {
          auto error = ERR_get_error();
          throw crypto_exception("EVP_CipherFinal_ex failed", error);
        }

        input_readed = 0;
        output_written = buf_len;
      }
      else {
        input_readed = 0;
        output_written = 0;
      }
    }

  private:
    EVP_CIPHER_CTX * ctx_;
    bool eof_;
    size_t block_size_;
  };
  
}

#endif // __VDS_CRYPTO_SYMMETRICCRYPTO_P_H_
