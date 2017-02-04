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
    
  };
  
  class asymmetric_private_key
  {
  public:
    asymmetric_private_key();
    asymmetric_private_key(const asymmetric_crypto_info & info);
    ~asymmetric_private_key();
    
    void generate();

    void load(const filename & filename);

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
    asymmetric_public_key(const asymmetric_private_key & key);
    ~asymmetric_public_key();

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

    const unsigned char * signature() const {
      return this->sig_;
    }

    size_t signature_length() const {
      return this->sig_len_;
    }

  private:
    EVP_MD_CTX * ctx_;
    const EVP_MD * md_;
    unsigned char * sig_;
    size_t sig_len_;
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
      const unsigned char * sig,
      size_t sig_len
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
    
    int update(
      const void * data,
      int len,
      void * result_data,
      int result_data_len);
  };
  
  class asymmetric_decrypt
  {
  public:
    asymmetric_decrypt(const asymmetric_private_key & key);
  };
  
  //http://www.codepool.biz/how-to-use-openssl-to-sign-certificate.html
  //openssl genrsa -out cakey.pem 2048
  //openssl req -new -days 365 -x509 -key cakey.pem -out cacert.pem
  //- nodes - subj / C = CA / ST = BC / L = Vancouver / O = Dynamsoft / OU = Dynamsoft / CN = Dynamsoft / emailAddress = support@dynamsoft.com
  //openssl rsa -in cakey.pem -pubout -out ca_pub.key
  class certificate
  {
  public:
    certificate();
    ~certificate();
    
    void load(const filename & filename);
    void save(const filename & filename);

    X509 * cert() const
    {
      return this->cert_;
    }

  private:
    X509 * cert_;
  };
}

#endif // __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
