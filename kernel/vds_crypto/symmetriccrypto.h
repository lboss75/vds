#ifndef __VDS_CRYPTO_SYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_SYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class symmetric_encrypt;
  class symmetric_decrypt;
  class _symmetric_encrypt;
  class _symmetric_crypto_info;
  class _symmetric_decrypt;
  
  class symmetric_crypto_info
  {
  public:
    size_t key_size() const;
    size_t iv_size() const;
    
  private:
    friend class symmetric_crypto;
    friend class _symmetric_encrypt;
    friend class _symmetric_decrypt;
    symmetric_crypto_info(_symmetric_crypto_info * impl);
    
    _symmetric_crypto_info * impl_;
  };
  
  class symmetric_crypto
  {
  public:
    static const symmetric_crypto_info & aes_256_cbc();
  };
  
  class symmetric_key
  {
  public:
    symmetric_key(const symmetric_crypto_info & crypto_info);
    
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
    friend class _symmetric_encrypt;
    friend class _symmetric_decrypt;
    
    const symmetric_crypto_info & crypto_info_;
    std::unique_ptr<unsigned char> key_;
    std::unique_ptr<unsigned char> iv_;
  };
  
  class _symmetric_encrypt;
  class symmetric_encrypt
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
    _symmetric_encrypt * impl_;
  };
  
  class _symmetric_decrypt;
  class symmetric_decrypt
  {
  public:
    symmetric_decrypt(const symmetric_key & key);
    ~symmetric_decrypt();
    
    size_t update(
      const void * data,
      size_t len,
      void * result_data,
      size_t result_data_len);
    
  private:
    _symmetric_decrypt * impl_;
  };
  
}

#endif // __SYMMETRICCRYPTO_H_
