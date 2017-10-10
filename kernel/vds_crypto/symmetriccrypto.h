#ifndef __VDS_CRYPTO_SYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_SYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "binary_serialize.h"

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

    size_t block_size() const;
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
    static const symmetric_crypto_info & rc4();
  };
  
  class symmetric_key
  {
  public:
    symmetric_key(const symmetric_crypto_info & crypto_info);
    symmetric_key(const symmetric_crypto_info & crypto_info, binary_deserializer & s);
    symmetric_key(const symmetric_crypto_info & crypto_info, binary_deserializer && s);
    symmetric_key(const symmetric_key & origin);
    symmetric_key(symmetric_key && origin);
    ~symmetric_key();
    
    symmetric_key & operator = (const symmetric_key & origin) = delete;
    symmetric_key & operator = (symmetric_key && origin) = delete;
    
    void generate();
    
    const unsigned char * key() const {
      return this->key_;
    }
    
    const unsigned char * iv() const {
      return this->iv_;
    }

    void serialize(binary_serializer & s) const;

    size_t block_size() const;
    
    static symmetric_key from_password(const std::string & password);
    
  private:
    friend class symmetric_encrypt;
    friend class symmetric_decrypt;
    friend class _symmetric_encrypt;
    friend class _symmetric_decrypt;
    
    symmetric_key(const symmetric_crypto_info & crypto_info, unsigned char * key, unsigned char * iv);
    
    const symmetric_crypto_info & crypto_info_;
    unsigned char * key_;
    unsigned char * iv_;
  };
  
  class symmetric_encrypt
  {
  public:
    symmetric_encrypt(
      const symmetric_key & key);
    ~symmetric_encrypt();

    void update(
      const void * input_buffer,
      size_t input_buffer_size,
      void * output_buffer,
      size_t output_buffer_size,
      size_t & input_readed,
      size_t & output_written);
    
    static const_data_buffer encrypt(
      const symmetric_key & key,
      const void * input_buffer,
      size_t input_buffer_size);
    
  private:
    _symmetric_encrypt * const impl_;
  };
  
  class symmetric_decrypt
  {
  public:
    symmetric_decrypt(
      const symmetric_key & key);
    
    ~symmetric_decrypt();

    void update(
      const void * input_buffer,
      size_t input_buffer_size,
      void * output_buffer,
      size_t output_buffer_size,
      size_t & input_readed,
      size_t & output_written);

    static const_data_buffer decrypt(
      const symmetric_key & key,
      const void * input_buffer,
      size_t input_buffer_size);
    
  private:
      _symmetric_decrypt * const impl_;
  };
  
}

#endif // __SYMMETRICCRYPTO_H_
