#ifndef __VDS_CRYPTO_ASYMMETRICCRYPTO_P_H_
#define __VDS_CRYPTO_ASYMMETRICCRYPTO_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "asymmetriccrypto.h"
#include "crypto_exception.h"

namespace vds {
  class _asymmetric_sign;
  class _asymmetric_public_key;
  class _asymmetric_sign_verify;

  class _asymmetric_private_key
  {
  public:
    _asymmetric_private_key(EVP_PKEY * key);
    _asymmetric_private_key(const asymmetric_crypto_info & info);
    ~_asymmetric_private_key();
    
    void generate();

    static asymmetric_private_key parse(const std::string & value, const std::string & password = std::string());
    std::string str(const std::string & password = std::string()) const;
    
    vds::const_data_buffer der(const std::string &password) const;
    static asymmetric_private_key parse_der(
      const const_data_buffer & value,

      const std::string & password /*= std::string()*/);

    void load(const filename & filename, const std::string & password = std::string());
    void save(const filename & filename, const std::string & password = std::string()) const;

    EVP_PKEY * key() const
    {
      return this->key_;
    }

    const_data_buffer decrypt(const void * data, size_t size)
    {
      size_t blocksize = (size_t)RSA_size(EVP_PKEY_get1_RSA(this->key_));
      std::vector<uint8_t> result;

      auto p = (const uint8_t *)data;
      auto l = size;

      uint8_t * buffer = new uint8_t[RSA_size(EVP_PKEY_get1_RSA(this->key_))];
      while (l > 0) {
        auto n = l;
        if (n > blocksize) {
          n = blocksize;
        }

        auto len = RSA_private_decrypt(safe_cast<int>(n), p, buffer, EVP_PKEY_get1_RSA(this->key_), RSA_PKCS1_PADDING);
        if (0 > len) {
          auto error = ERR_get_error();
          delete[] buffer;
          throw crypto_exception("RSA_private_decrypt failed", error);
        }

        result.insert(result.end(), buffer, buffer + len);

        p += n;
        l -= n;
      }
      
      delete[] buffer;

      return const_data_buffer(result.data(), result.size());
    }

  private:
    friend class _asymmetric_sign;
    friend class _asymmetric_public_key;

    const asymmetric_crypto_info & info_;
    EVP_PKEY_CTX * ctx_;
    EVP_PKEY * key_;
  };

  class _asymmetric_public_key
  {
  public:
    _asymmetric_public_key(EVP_PKEY * key);
    _asymmetric_public_key(const asymmetric_private_key & key);
    ~_asymmetric_public_key();

    EVP_PKEY * key() const
    {
      return this->key_;
    }

    const_data_buffer der() const;
    static asymmetric_public_key parse_der(const const_data_buffer& value);

    static asymmetric_public_key parse(const std::string & format);
    std::string str() const;

    void load(const filename & filename);
    void save(const filename & filename) const;

    const_data_buffer encrypt(const void * data, size_t data_size)
    {
      size_t blocksize = (size_t)RSA_size(EVP_PKEY_get1_RSA(this->key_)) - 11;// EVP_PKEY_size(this->key_);
      std::vector<uint8_t> result;

      auto p = reinterpret_cast<const uint8_t *>(data);
      auto l = data_size;

      uint8_t * buffer = new uint8_t[RSA_size(EVP_PKEY_get1_RSA(this->key_))];
      while (l > 0) {
        auto n = l;
        if (n > blocksize) {
          n = blocksize;
        }

        auto len = RSA_public_encrypt(safe_cast<int>(n), p, buffer, EVP_PKEY_get1_RSA(this->key_), RSA_PKCS1_PADDING);
        if (0 > len) {
          auto error = ERR_get_error();
          delete[] buffer;
          throw crypto_exception("RSA_private_encrypt failed", error);
        }

        result.insert(result.end(), buffer, buffer + len);

        p += n;
        l -= n;
      }
      delete[] buffer;

      return const_data_buffer(result.data(), result.size());
    }

  private:
    friend class _asymmetric_sign_verify;
    
    const asymmetric_crypto_info & info_;
    EVP_PKEY * key_;
  };

  class _asymmetric_sign
  {
  public:
    _asymmetric_sign(
      const hash_info & hash_info,
      const asymmetric_private_key & key
    );
    ~_asymmetric_sign();

    std::future<void> write_async(
      const service_provider *sp,
      const uint8_t *data,
      size_t len);

    const const_data_buffer & signature() const {
      return this->sig_;
    }

  private:
    EVP_MD_CTX * ctx_;
    const EVP_MD * md_;
    const_data_buffer sig_;
  };

  class _asymmetric_sign_verify
  {
  public:
    _asymmetric_sign_verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const const_data_buffer & sig);
    
    ~_asymmetric_sign_verify();

    std::future<void> write_async(
      const service_provider *sp,
      const uint8_t *data,
      size_t len);

    bool result() const { return this->result_; }

  private:
    const_data_buffer sig_;
    EVP_MD_CTX * ctx_;
    const EVP_MD * md_;
    bool result_;
  };
  
  
  //http://www.codepool.biz/how-to-use-openssl-to-sign-certificate.html
  //openssl genrsa -out cakey.pem 2048
  //openssl req -new -days 365 -x509 -key cakey.pem -out cacert.crt
  //openssl rsa -in cakey.pem -pubout -out ca_pub.key
  //
  // openssl genrsa -out user.key 2048
  // openssl req -new -key user.key -out user.csr
  // openssl x509 -req -days 730 -in user.csr -CA cacert.crt -CAkey cakey.pem -CAcreateserial -out user.crt
  
  class _certificate
  {
  public:
    _certificate(certificate && original);
    _certificate(X509 * cert);
    ~_certificate();

    std::string str() const;
    const_data_buffer der() const;

    void load(const filename & filename);
    void save(const filename & filename) const;

    X509 * cert() const
    {
      return this->cert_;
    }

    std::string subject() const;
    std::string issuer() const;
    const_data_buffer fingerprint(const hash_info & hash_algo = hash::sha256()) const;

    static certificate create_new(
      const asymmetric_public_key & new_certificate_public_key,
      const asymmetric_private_key & new_certificate_private_key,
      const certificate::create_options & options
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
  
  class _certificate_store
  {
  public:
    _certificate_store();
    ~_certificate_store();

    void add(const certificate & cert);
    void load_locations(const std::string & location);
    
    certificate_store::verify_result verify(const certificate & cert) const;
    
  private:
    X509_STORE * store_;
  };
}

#endif // __VDS_CRYPTO_ASYMMETRICCRYPTO_P_H_
