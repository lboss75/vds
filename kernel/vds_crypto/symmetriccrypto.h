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
    symmetric_key(const symmetric_key & origin);
    
    void generate();
    
    const unsigned char * key() const {
      return this->key_.get();
    }
    
    const unsigned char * iv() const {
      return this->iv_.get();
    }

    void serialize(binary_serializer & s);

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
    symmetric_encrypt(const symmetric_key & key);

    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const symmetric_encrypt & args)
        : base_class(context),
        impl_(args.create_implementation())
      {
      }

      void operator()(const void * data, size_t size)
      {
        size_t out_size = sizeof(this->buffer);
        if (data_update(this->impl_, data, size, this->buffer, out_size))
        {
          this->next(this->buffer, out_size);
        }
        else
        {
          this->prev();
        }
      }

      void processed()
      {
        size_t out_size = sizeof(this->buffer);
        if (data_processed(this->impl_, this->buffer, out_size))
        {
          this->next(this->buffer, out_size);
        }
        else
        {
          this->prev();
        }
      }

    private:
      _symmetric_encrypt * impl_;
      uint8_t buffer[1024];
    };
    
  private:
    symmetric_key key_;
    _symmetric_encrypt * create_implementation() const;

    static bool data_update(
      _symmetric_encrypt * impl,
      const void * data,
      size_t len,
      void * result_data,
      size_t & result_data_len);

    static bool data_processed(
      _symmetric_encrypt * impl,
      void * result_data,
      size_t & result_data_len);
  };
  
  class symmetric_decrypt
  {
  public:
    symmetric_decrypt(const symmetric_key & key);


    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const symmetric_decrypt & args)
        : base_class(context),
        impl_(args.create_implementation())
      {

      }

      void operator()(const void * data, size_t size)
      {
        size_t out_size = sizeof(this->buffer);
        if (data_update(this->impl_, data, size, this->buffer, out_size))
        {
          this->next(this->buffer, out_size);
        }
        else
        {
          this->prev();
        }
      }

      void processed()
      {
        size_t out_size = sizeof(this->buffer);
        if (data_processed(this->impl_, this->buffer, out_size))
        {
          this->next(this->buffer, out_size);
        }
        else
        {
          this->prev();
        }
      }

    private:
      _symmetric_decrypt * impl_;
      uint8_t buffer[1024];
    };

  private:
    symmetric_key key_;
    _symmetric_decrypt * create_implementation() const;

    static bool data_update(
      _symmetric_decrypt * impl,
      const void * data,
      size_t len,
      void * result_data,
      size_t & result_data_len);

    static bool data_processed(
      _symmetric_decrypt * impl,
      void * result_data,
      size_t & result_data_len);
  };
  
}

#endif // __SYMMETRICCRYPTO_H_
