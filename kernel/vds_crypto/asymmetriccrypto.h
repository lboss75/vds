#ifndef __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
#define __VDS_CRYPTO_ASYMMETRICCRYPTO_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include <utility>

#include "hash.h"
#include "filename.h"
#include "crypto_service.h"
#include "stream.h"
#include "binary_serialize.h"
#include "const_data_buffer.h"

namespace vds {
  class _asymmetric_sign;
  class _asymmetric_sign_verify;
  class _asymmetric_public_key;
  class _certificate;
  class _ssl_tunnel;
  class _certificate_store;
  struct hash_info;
  
  struct asymmetric_crypto_info
  {
    int id;
    int key_bits;
  };
  
  class asymmetric_crypto
  {
  public:
    static const asymmetric_crypto_info & rsa2048();
    static const asymmetric_crypto_info & rsa4096();
  };
  
  class _asymmetric_private_key;
  class asymmetric_private_key
  {
  public:
    asymmetric_private_key();
    asymmetric_private_key(const asymmetric_private_key & original) = delete;
    asymmetric_private_key(asymmetric_private_key && original) noexcept;
    ~asymmetric_private_key();

    expected<void> create(const asymmetric_crypto_info & info);

    static expected<asymmetric_private_key> generate(const asymmetric_crypto_info & info);

    static expected<asymmetric_private_key> parse(const std::string & value, const std::string & password = std::string());
    expected<std::string> str(const std::string & password = std::string()) const;
    
    expected<const_data_buffer> der(const std::string &password) const;
    static expected<asymmetric_private_key> parse_der(
      const const_data_buffer & value,
      const std::string & password);

    expected<void> load(const filename & filename, const std::string & password = std::string());
    expected<void> save(const filename & filename, const std::string & password = std::string()) const;

    expected<const_data_buffer> decrypt(const const_data_buffer & data) const;
    expected<const_data_buffer> decrypt(const void * data, size_t size) const;
    
    asymmetric_private_key & operator = (asymmetric_private_key && original);

  private:
    friend class _asymmetric_sign;
    friend class _asymmetric_private_key;
    friend class _asymmetric_public_key;
    friend class _certificate;
    friend class _ssl_tunnel;
    
    asymmetric_private_key(_asymmetric_private_key * impl);
    
    _asymmetric_private_key * impl_;
  };

  class _asymmetric_public_key;
  class asymmetric_public_key
  {
  public:
    asymmetric_public_key();
    asymmetric_public_key(const asymmetric_public_key & original) = delete;
    asymmetric_public_key(asymmetric_public_key && original);
    ~asymmetric_public_key();

    static expected<asymmetric_public_key> create(const asymmetric_private_key & key);

    static expected<asymmetric_public_key> parse(const std::string & format);
    expected<std::string> str() const;

    static expected<asymmetric_public_key> parse_der(const const_data_buffer & value);
    expected<const_data_buffer> der() const;

    expected<void> load(const filename & filename);
    expected<void> save(const filename & filename);

    expected<const_data_buffer> encrypt(const const_data_buffer & data);
    expected<const_data_buffer> encrypt(const void * data, size_t data_size);
    expected<const_data_buffer> encrypt(expected<const_data_buffer> && data) {
      if(data.has_error()) {
        return unexpected(std::move(data.error()));
      }

      return this->encrypt(data.value());
    }

    asymmetric_public_key & operator = (asymmetric_public_key && original) noexcept;

  private:
    asymmetric_public_key(_asymmetric_public_key * impl);
    
    friend class _asymmetric_sign_verify;
    friend class _certificate;
    friend class _asymmetric_public_key;

    _asymmetric_public_key * impl_;
  };

  class asymmetric_sign : public stream_output_async<uint8_t>
  {
  public:
    asymmetric_sign();

    ~asymmetric_sign();

    expected<void> create(
      const hash_info & hash_info,
      const asymmetric_private_key & key);

    expected<const_data_buffer> signature();

    static expected<const_data_buffer> signature(
      const hash_info & hash_info,
      const asymmetric_private_key & key,
      const const_data_buffer & data);

    static expected<const_data_buffer> signature(
      const hash_info & hash_info,
      const asymmetric_private_key & key,
      const void * data,
      size_t data_size);

    vds::async_task<vds::expected<void>> write_async(
      
      const uint8_t  *data,
      size_t len) override;

  private:
    _asymmetric_sign * impl_;
  };

  class asymmetric_sign_verify : public stream_output_async<uint8_t>
  {
  public:
    asymmetric_sign_verify();

    ~asymmetric_sign_verify();

    expected<void> create(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const const_data_buffer & sig);

    expected<bool> result() const;
    
    static expected<bool> verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const const_data_buffer & signature,
      const void * data,
      size_t data_size);

    static expected<bool> verify(
      const hash_info & hash_info,
      const asymmetric_public_key & key,
      const const_data_buffer & signature,
      const const_data_buffer & data);

    vds::async_task<vds::expected<void>> write_async(      
      const uint8_t *data,
      size_t len) override;

  private:
    _asymmetric_sign_verify * impl_;
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
    certificate_extension()
      : oid(0)
    {
    }

    certificate_extension(
      crypto_service::certificate_extension_type extension_type,
      std::string && val)
    : oid(extension_type), value(std::move(val))
    {
    }

    crypto_service::certificate_extension_type oid;
    std::string value;
  };

  class _certificate;
  class certificate
  {
  public:
    certificate();
    certificate(const certificate & original) = delete;
    certificate(certificate && original);
    ~certificate();

    static expected<certificate> parse(const std::string & format);
    expected<std::string> str() const;

    static expected<certificate> parse_der(const const_data_buffer & body);
    expected<const_data_buffer> der() const;

    expected<void> load(const filename & filename);
    expected<void> save(const filename & filename) const;

    std::string subject() const;
    std::string issuer() const;
    expected<const_data_buffer> fingerprint(const hash_info & hash_algo = hash::sha256()) const;

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

    static expected<certificate> create_new(
      const asymmetric_public_key & new_certificate_public_key,
      const asymmetric_private_key & new_certificate_private_key,
      const create_options & options
    );

    expected<asymmetric_public_key> public_key() const;
    
    bool is_ca_cert() const;
    
    bool is_issued(const certificate & issuer) const;
    
    int extension_count() const;
    int extension_by_NID(int nid) const;
    expected<certificate_extension> get_extension(int index) const;

    certificate & operator = (const certificate & original) = delete;
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
    certificate_store(_certificate_store * impl);
    certificate_store(const certificate_store &) = delete;
    certificate_store(certificate_store &&) noexcept;
    ~certificate_store();

    static expected<certificate_store> create();

    expected<void> add(const certificate & cert);
    expected<void> load_locations(const std::string & location);
    
    struct verify_result
    {
      int error_code;
      std::string error;
      std::string issuer;
    };
    
    expected<verify_result> verify(const certificate & cert) const;
    certificate_store & operator = (const certificate_store &) = delete;
    certificate_store & operator = (certificate_store &&) noexcept;

  private:
    _certificate_store * impl_;
  };

    inline vds::expected<void> operator <<(vds::binary_serializer & s, const std::shared_ptr<vds::certificate> & cert)
    {
        GET_EXPECTED(der, cert->der());
        return (s << der);
    }

    inline vds::expected<void> operator >>(vds::binary_deserializer & s, std::shared_ptr<vds::certificate> & cert)
    {
        vds::const_data_buffer cert_data;
        CHECK_EXPECTED(s.get(cert_data));
        GET_EXPECTED(der, vds::certificate::parse_der(cert_data));
        cert = std::make_shared<vds::certificate>(std::move(der));
        return expected<void>();
    }

    inline vds::expected<void> operator <<(vds::binary_serializer & s, const std::shared_ptr<vds::asymmetric_private_key> & key)
    {
        GET_EXPECTED(der, key->der(std::string()));
        return (s << der);
    }

    inline vds::expected<void> operator >>(vds::binary_deserializer & s, std::shared_ptr<vds::asymmetric_private_key> & key)
    {
        vds::const_data_buffer key_data;
        CHECK_EXPECTED(s >> key_data);
        GET_EXPECTED(der, vds::asymmetric_private_key::parse_der(key_data, std::string()));
        key = std::make_shared<vds::asymmetric_private_key>(std::move(der));
        return expected<void>();
    }
}


#endif // __VDS_CRYPTO_ASYMMETRICCRYPTO_H_
