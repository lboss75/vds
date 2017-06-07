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
  };
  
  class symmetric_key
  {
  public:
    symmetric_key(const symmetric_crypto_info & crypto_info);
    symmetric_key(const symmetric_crypto_info & crypto_info, binary_deserializer & s);
    symmetric_key(const symmetric_crypto_info & crypto_info, binary_deserializer && s);
    symmetric_key(const symmetric_key & origin);
    
    void generate();
    
    const unsigned char * key() const {
      return this->key_.get();
    }
    
    const unsigned char * iv() const {
      return this->iv_.get();
    }

    void serialize(binary_serializer & s) const;

    size_t block_size() const;
    
  private:
    friend class symmetric_encrypt;
    friend class symmetric_decrypt;
    friend class _symmetric_encrypt;
    friend class _symmetric_decrypt;
    
    const symmetric_crypto_info & crypto_info_;
    std::unique_ptr<unsigned char> key_;
    std::unique_ptr<unsigned char> iv_;
  };
  
  class symmetric_encrypt
  {
  public:
    symmetric_encrypt(
      const symmetric_key & key);

    using incoming_item_type = uint8_t;
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 32;
    
    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_filter<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const symmetric_encrypt & args)
        : base_class(context),
        impl_(args.create_implementation())
      {
      }

      void sync_process_data(
        const vds::service_provider & sp,
        size_t & input_readed,
        size_t & output_written)
      {
        data_update(
          this->impl_,
          this->input_buffer(),
          this->input_buffer_size(),
          this->output_buffer(),
          this->output_buffer_size(),
          input_readed,
          output_written);
      }

    private:
      _symmetric_encrypt * impl_;
    };
    
  private:
    symmetric_key key_;
    _symmetric_encrypt * create_implementation() const;

    static void data_update(
      _symmetric_encrypt * impl,
      const uint8_t * input_buffer,
      size_t input_buffer_size,
      uint8_t * output_buffer,
      size_t output_buffer_size,
      size_t & input_readed,
      size_t & output_written);
  };
  
  class symmetric_decrypt
  {
  public:
    symmetric_decrypt(
      const symmetric_key & key);


    using incoming_item_type = uint8_t;
    using outgoing_item_type = uint8_t;
    static constexpr size_t BUFFER_SIZE = 1024;
    static constexpr size_t MIN_BUFFER_SIZE = 32;
    
    template<typename context_type>
    class handler : public vds::sync_dataflow_filter<context_type, handler<context_type>>
    {
      using base_class = vds::sync_dataflow_filter<context_type, handler<context_type>>;
    public:
      handler(
        const context_type & context,
        const symmetric_decrypt & args)
        : base_class(context),
        impl_(args.create_implementation())
      {
      }

      void sync_process_data(
        const vds::service_provider & sp,
        size_t & input_readed,
        size_t & output_written)
      {
        data_update(
          this->impl_,
          this->input_buffer(),
          this->input_buffer_size(),
          this->output_buffer(),
          this->output_buffer_size(),
          input_readed,
          output_written);
      }

    private:
      _symmetric_decrypt * impl_;
    };

  private:
    symmetric_key key_;

    _symmetric_decrypt * create_implementation() const;
    static void data_update(
      _symmetric_decrypt * impl,
      const uint8_t * input_buffer,
      size_t input_buffer_size,
      uint8_t * output_buffer,
      size_t output_buffer_size,
      size_t & input_readed,
      size_t & output_written);
  };
  
}

#endif // __SYMMETRICCRYPTO_H_
