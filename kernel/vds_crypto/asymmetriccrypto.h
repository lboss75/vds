#ifndef __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_ASYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "hash.h"

namespace vds {
  
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
  
  class asymmetric_private_key
  {
  public:
    asymmetric_private_key();
    asymmetric_private_key(EVP_PKEY * key);
    asymmetric_private_key(asymmetric_private_key && original);
    asymmetric_private_key(const asymmetric_crypto_info & info);
    ~asymmetric_private_key();
    
    void generate();

    static asymmetric_private_key parse(const std::string & value, const std::string & password = std::string());
    std::string str(const std::string & password = std::string()) const;

    void load(const filename & filename, const std::string & password = std::string());
    void save(const filename & filename, const std::string & password = std::string()) const;

    EVP_PKEY * key() const
    {
      return this->key_;
    }
        
  private:
    friend class asymmetric_sign;
    friend class asymmetric_public_key;

    const asymmetric_crypto_info & info_;
    EVP_PKEY_CTX * ctx_;
    EVP_PKEY * key_;
  };

  class asymmetric_public_key
  {
  public:
    asymmetric_public_key(EVP_PKEY * key);
    asymmetric_public_key(asymmetric_public_key && original);
    asymmetric_public_key(const asymmetric_private_key & key);
    ~asymmetric_public_key();

    EVP_PKEY * key() const
    {
      return this->key_;
    }

    static asymmetric_public_key parse(const std::string & format);
    std::string str() const;

    void load(const filename & filename);
    void save(const filename & filename);

  private:
    friend class asymmetric_sign_verify;
    const asymmetric_crypto_info & info_;
    EVP_PKEY * key_;
  };

  class asymmetric_sign
  {
  public:
    asymmetric_sign(
      const hash_info & hash_info,
      const asymmetric_private_key & key
    );
    ~asymmetric_sign();

    void update(
      const void * data,
      int len);

    void final();

    const data_buffer & signature() const {
      return this->sig_;
    }

  private:
    EVP_MD_CTX * ctx_;
    const EVP_MD * md_;
    data_buffer sig_;
  };

  class asymmetric_sign_verify
  {
  public:
    asymmetric_sign_verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key
    );
    ~asymmetric_sign_verify();

    void update(
      const void * data,
      int len);

    bool verify(
      const data_buffer & sig
    );

  private:
    EVP_MD_CTX * ctx_;
    const EVP_MD * md_;
  };
  
  class asymmetric_encrypt
  {
  public:
    asymmetric_encrypt(
      const asymmetric_public_key & key
    );
    
    size_t update(
      const void * data,
      size_t len,
      void * result_buffer,
      size_t result_buffer_len);
    
    size_t final(
      void * result_buffer,
      size_t result_buffer_len);

    template <typename context_type>
    class handler : public sequence_step<context_type, void(const void * data, size_t len)>
    {
      using base_class = sequence_step<context_type, void(const void * data, size_t len)>;
    public:
      handler(
        const context_type & context,
        const asymmetric_encrypt & key)
        : base_class(context)
      {
      }

      void operator()(const void * data, size_t len)
      {

      }

    private:
      asymmetric_encrypt encrypt_;
    };
  };
  
  class asymmetric_decrypt
  {
  public:
    asymmetric_decrypt(const asymmetric_private_key & key);
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

  class certificate
  {
  public:
    certificate();
    certificate(certificate && original);
    certificate(X509 * cert);
    ~certificate();

    static certificate parse(const std::string & format);
    std::string str() const;

    void load(const filename & filename);
    void save(const filename & filename) const;

    X509 * cert() const
    {
      return this->cert_;
    }

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

  private:
    X509 * cert_;

    static bool add_ext(X509 * cert, int nid, const char *value);
  };
  
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
    X509_STORE * store_;
  };
}

#endif // __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
