#ifndef __VDS_CRYPTO_SYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_SYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "binary_serialize.h"
#include "stream.h"

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
    symmetric_key();
    symmetric_key(const symmetric_key & origin) = delete;
    symmetric_key(symmetric_key && origin);
    ~symmetric_key();
    
    symmetric_key & operator = (const symmetric_key & origin) = delete;
    symmetric_key & operator = (symmetric_key && origin) = default;
    
    static symmetric_key generate(const symmetric_crypto_info & crypto_info);
    static symmetric_key deserialize(const symmetric_crypto_info & crypto_info, binary_deserializer & s);
    static symmetric_key deserialize(const symmetric_crypto_info & crypto_info, binary_deserializer && s);

    void serialize(binary_serializer & s) const;

    const_data_buffer serialize() const{
      binary_serializer s;
      this->serialize(s);
      return s.move_data();
    }

    size_t block_size() const;
    
    static symmetric_key from_password(const std::string & password);

    static symmetric_key create(
        const symmetric_crypto_info & crypto_info,
        const uint8_t * key,
        const uint8_t * iv);

  private:
    friend class _symmetric_encrypt;
    friend class _symmetric_decrypt;
    friend class _symmetric_key;

    symmetric_key(class _symmetric_key * impl);

    class _symmetric_key * impl_;
  };
  
  class symmetric_encrypt : public stream_output_async<uint8_t>
  {
  public:
    symmetric_encrypt(
      const symmetric_key & key,
      const std::shared_ptr<stream_output_async<uint8_t>> & target);

    ~symmetric_encrypt();

    static const_data_buffer encrypt(
      const symmetric_key & key,
      const void * input_buffer,
      size_t input_buffer_size);

    static const_data_buffer encrypt(
      const service_provider & sp,
      const symmetric_key & key,
        const const_data_buffer & input_buffer){
      return encrypt(key, input_buffer.data(), input_buffer.size());
    }

  private:
    _symmetric_encrypt * impl_;
  };
  
  class symmetric_decrypt : public stream_output_async<uint8_t>
  {
  public:
    symmetric_decrypt(
      const symmetric_key & key,
      const std::shared_ptr<stream_output_async<uint8_t>> & target);

    ~symmetric_decrypt();

    static const_data_buffer decrypt(
      const symmetric_key & key,
      const void * input_buffer,
      size_t input_buffer_size);

    static const_data_buffer decrypt(
      const symmetric_key & key,
        const const_data_buffer & input_buffer){
      return decrypt(key, input_buffer.data(), input_buffer.size());
    }

  private:
    _symmetric_decrypt * impl_;
  };

}

#endif // __SYMMETRICCRYPTO_H_
