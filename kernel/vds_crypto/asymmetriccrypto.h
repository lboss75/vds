#ifndef __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_ASYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "hash.h"

namespace vds {
  class _asymmetric_sign;
  class _asymmetric_sign_verify;
  class _asymmetric_public_key;
  class _certificate;
  class _ssl_tunnel;
  class _certificate_store;
  
  struct asymmetric_crypto_info
  {
    int id;
    int key_bits;
  };
  
  class asymmetric_crypto
  {
  public:
    static const asymmetric_crypto_info & unknown();
    static const asymmetric_crypto_info & rsa2048();
    static const asymmetric_crypto_info & rsa4096();
  };
  
  class _asymmetric_private_key;
  class asymmetric_private_key
  {
  public:
    asymmetric_private_key();
    asymmetric_private_key(const asymmetric_private_key & original);
    asymmetric_private_key(const asymmetric_crypto_info & info);
    ~asymmetric_private_key();
    
    void generate();

    static asymmetric_private_key parse(const std::string & value, const std::string & password = std::string());
    std::string str(const std::string & password = std::string()) const;

    void load(const filename & filename, const std::string & password = std::string());
    void save(const filename & filename, const std::string & password = std::string()) const;

    data_buffer decrypt(const data_buffer & data);

  private:
    friend class _asymmetric_sign;
    friend class _asymmetric_public_key;
    friend class _certificate;
    friend class _ssl_tunnel;
    
    asymmetric_private_key(_asymmetric_private_key * impl);
    
    std::shared_ptr<_asymmetric_private_key> impl_;
  };

  class _asymmetric_public_key;
  class asymmetric_public_key
  {
  public:
    asymmetric_public_key(const asymmetric_public_key & original);
    asymmetric_public_key(const asymmetric_private_key & key);
    ~asymmetric_public_key();

    static asymmetric_public_key parse(const std::string & format);
    std::string str() const;

    void load(const filename & filename);
    void save(const filename & filename);

    data_buffer encrypt(const data_buffer & data);

  private:
    asymmetric_public_key(_asymmetric_public_key * impl);
    
    friend class _asymmetric_sign_verify;
    friend class _certificate;
    std::shared_ptr<_asymmetric_public_key> impl_;
  };

  class asymmetric_sign
  {
  public:
    asymmetric_sign(
      const hash_info & hash_info,
      const asymmetric_private_key & key);

    template <typename context_type>
    class handler : public dataflow_step<context_type, void(const void *, size_t)>
    {
      using base_class = dataflow_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const asymmetric_sign & args)
        : base_class(context),
        impl_(args.create_implementation())
      {
      }

      void operator()(const void * data, size_t size)
      {
        if (0 == size) {
          data_final(this->impl_, this->signature_);
          this->next(this->signature_.data(), this->signature_.size());
        }
        else {
          data_update(this->impl_, data, size);
          this->prev();
        }
      }

    private:
      _asymmetric_sign * impl_;
      data_buffer signature_;
    };

    static data_buffer signature(
      const hash_info & hash_info,
      const asymmetric_private_key & key,
      const data_buffer & data);

  private:
    const hash_info & hash_info_;
    asymmetric_private_key key_;
    _asymmetric_sign * create_implementation() const;

    static void data_update(
      _asymmetric_sign * impl,
      const void * data,
      int len);

    static void data_final(_asymmetric_sign * impl, data_buffer & result);
  };

  class _asymmetric_sign_verify;
  class asymmetric_sign_verify
  {
  public:
    asymmetric_sign_verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const data_buffer & sign);

    template <typename context_type>
    class handler : public dataflow_step<context_type, void(bool)>
    {
      using base_class = dataflow_step<context_type, void(bool)>;
    public:
      handler(
        const context_type & context,
        const asymmetric_sign_verify & args)
        : base_class(context),
        impl_(args.create_implementation()),
        signature_(args.signature_)
      {
      }

      void operator()(const void * data, size_t size)
      {
        if (0 == size) {
          auto result = data_final(this->impl_, this->signature_);
          this->next(result);
        }
        else {
          data_update(this->impl_, data, size);
          this->prev();
        }
      }

    private:
      _asymmetric_sign_verify * impl_;
      data_buffer signature_;
    };
    
    static bool verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const data_buffer & signature,
      const void * data,
      size_t data_size);
    
  private:
    const hash_info & hash_info_;
    asymmetric_public_key key_;
    data_buffer signature_;

    _asymmetric_sign_verify * create_implementation() const;

    static void data_update(
      _asymmetric_sign_verify * impl,
      const void * data,
      int len);

    static bool data_final(
      _asymmetric_sign_verify * impl,
      const data_buffer & signature);

  };
  
  
  //http://www.codepool.biz/how-to-use-openssl-to-sign-certificate.html
  //openssl genrsa -out cakey.pem 2048
  //openssl req -new -days 365 -x509 -key cakey.pem -out cacert.crt
  //openssl rsa -in cakey.pem -pubout -out ca_pub.key
  //
  // openssl genrsa -out user.key 2048
  // openssl req -new -key user.key -out user.csr
  // openssl x509 -req -days 730 -in user.csr -CA cacert.crt -CAkey cakey.pem -CAcreateserial -out user.crt
  
  struct certificate_extension
  {
    certificate_extension();

    std::string oid;
    std::string name;
    std::string description;
    std::string value;

    int base_nid;
  };

  class _certificate;
  class certificate
  {
  public:
    certificate();
    certificate(certificate && original);
    ~certificate();

    static certificate parse(const std::string & format);
    std::string str() const;

    void load(const filename & filename);
    void save(const filename & filename) const;

    std::string subject() const;
    std::string issuer() const;
    data_buffer fingerprint(const hash_info & hash_algo = hash::sha256()) const;

    class create_options
    {
    public:
      create_options()
        : ca_certificate(nullptr),
          ca_certificate_private_key(nullptr)
      {
      }

      std::string country;
      std::string organization;
      std::string name;

      const certificate * ca_certificate;
      const asymmetric_private_key * ca_certificate_private_key;

      std::list<certificate_extension> extensions;
    };

    static certificate create_new(
      const asymmetric_public_key & new_certificate_public_key,
      const asymmetric_private_key & new_certificate_private_key,
      const create_options & options
    );

    asymmetric_public_key public_key() const;
    
    bool is_ca_cert() const;
    
    bool is_issued(const certificate & issuer) const;
    
    int extension_count() const;
    int extension_by_NID(int nid) const;
    certificate_extension get_extension(int index) const;

    certificate & operator = (certificate && original);

  private:
    friend class _certificate;
    friend class _ssl_tunnel;
    friend class _certificate_store;
    
    certificate(_certificate * impl);
    
    _certificate * impl_;
  };
  
  class _certificate_store;
  class certificate_store
  {
  public:
    certificate_store();
    ~certificate_store();

    void add(const certificate & cert);
    void load_locations(const std::string & location);
    
    struct verify_result
    {
      int error_code;
      std::string error;
      std::string issuer;
    };
    
    verify_result verify(const certificate & cert) const;
    
  private:
    _certificate_store * impl_;
  };
}

#endif // __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
