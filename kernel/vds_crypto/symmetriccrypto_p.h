#ifndef __VDS_CRYPTO_SYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_SYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "symmetriccrypto.h"

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
    symmetric_encrypt(const symmetric_key & key);
    ~symmetric_encrypt();
    
    size_t update(
      const void * data,
      size_t len,
      void * result_data,
      size_t result_data_len);
    
  private:
    EVP_CIPHER_CTX * ctx_;
  };
  
  class _symmetric_decrypt
  {
  public:
    _symmetric_decrypt(const symmetric_key & key);
    ~_symmetric_decrypt();
    
    size_t update(
      const void * data,
      size_t len,
      void * result_data,
      size_t result_data_len);
    
  private:
    EVP_CIPHER_CTX * ctx_;
  };
  
}

#endif // __SYMMETRICCRYPTO_H_
