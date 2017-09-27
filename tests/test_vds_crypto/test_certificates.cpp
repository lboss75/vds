/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_certificates.h"
#include "test_config.h"

TEST(test_certificates, test_pem)
{
    vds::service_registrator registrator;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(
      test_config::instance().log_level(),
      test_config::instance().modules());

    registrator.add(console_logger);
    registrator.add(crypto_service);
    {
      auto sp = registrator.build("test_pem");
      registrator.start(sp);

      //Generate CA certificate
      std::string ca_certificate_text;
      std::string ca_private_key;

      {
        vds::asymmetric_private_key ca_certificate_private_key(vds::asymmetric_crypto::rsa2048());
        ca_certificate_private_key.generate();

        vds::asymmetric_public_key ca_certificate_public_key(ca_certificate_private_key);

        //Create CA certificate
        vds::certificate::create_options ca_options;
        ca_options.country = "RU";
        ca_options.organization = "Test Org";
        ca_options.name = "CA Cert";
        
        vds::certificate_extension ca_ext;
        ca_ext.oid = "1.2.3.4";
        ca_ext.name = "test_ext";
        ca_ext.description = "test";
        ca_ext.value = "test_value";
        
        ca_options.extensions.push_back(ca_ext);

        vds::certificate ca = vds::certificate::create_new(
          ca_certificate_public_key,
          ca_certificate_private_key,
          ca_options
        );

        ca_certificate_text = ca.str();
        ca_private_key = ca_certificate_private_key.str();
      }

      //Generate sub certificate
      std::string sub_certificate_text;
      std::string sub_private_key;
      {
        vds::asymmetric_private_key ca_certificate_private_key = vds::asymmetric_private_key::parse(ca_private_key);
        vds::certificate ca = vds::certificate::parse(ca_certificate_text);
        
        GTEST_ASSERT_EQ(ca.get_extension("test_ext").value, "test_value");

        vds::asymmetric_private_key sub_certificate_private_key(vds::asymmetric_crypto::rsa2048());
        sub_certificate_private_key.generate();

        vds::asymmetric_public_key sub_certificate_public_key(sub_certificate_private_key);

        //Create sub certificate
        vds::certificate::create_options sub_options;
        sub_options.country = "RU";
        sub_options.organization = "Test Org";
        sub_options.name = "Sub Cert";
        sub_options.ca_certificate = &ca;
        sub_options.ca_certificate_private_key = &ca_certificate_private_key;

        vds::certificate sub_certificate = vds::certificate::create_new(
          sub_certificate_public_key,
          sub_certificate_private_key,
          sub_options
        );

        sub_certificate_text = sub_certificate.str();
        sub_private_key = sub_certificate_private_key.str();
      }

      //Generate sub certificate
      std::string caudal_certificate_text;
      std::string caudal_private_key;
      {
        vds::asymmetric_private_key sub_certificate_private_key = vds::asymmetric_private_key::parse(sub_private_key);
        vds::certificate sub_certificate = vds::certificate::parse(sub_certificate_text);

        vds::asymmetric_private_key caudal_certificate_private_key(vds::asymmetric_crypto::rsa2048());
        caudal_certificate_private_key.generate();

        vds::asymmetric_public_key caudal_certificate_public_key(caudal_certificate_private_key);

        //Create sub certificate
        vds::certificate::create_options caudal_options;
        caudal_options.country = "RU";
        caudal_options.organization = "Test Org";
        caudal_options.name = "Caudal Cert";
        caudal_options.ca_certificate = &sub_certificate;
        caudal_options.ca_certificate_private_key = &sub_certificate_private_key;

        vds::certificate caudal_certificate = vds::certificate::create_new(
          caudal_certificate_public_key,
          caudal_certificate_private_key,
          caudal_options
        );

        caudal_certificate_text = caudal_certificate.str();
        caudal_private_key = caudal_certificate_private_key.str();
      }

      //Check
      {
        vds::certificate caudal_certificate = vds::certificate::parse(caudal_certificate_text);

        vds::certificate_store store;
        
        auto result = store.verify(caudal_certificate);
        ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
        ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=Sub Cert");
        ASSERT_TRUE(caudal_certificate.is_ca_cert());
      }
      {
        vds::certificate sub_certificate = vds::certificate::parse(sub_certificate_text);
        vds::certificate caudal_certificate = vds::certificate::parse(caudal_certificate_text);

        vds::certificate_store store;
        store.add(sub_certificate);
        
        auto result = store.verify(caudal_certificate);
        ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT);
        ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=CA Cert");
        ASSERT_TRUE(caudal_certificate.is_issued(sub_certificate));
        ASSERT_TRUE(caudal_certificate.is_ca_cert());
        ASSERT_TRUE(sub_certificate.is_ca_cert());
      }
      {
        vds::certificate ca_certificate = vds::certificate::parse(ca_certificate_text);
        vds::certificate caudal_certificate = vds::certificate::parse(caudal_certificate_text);

        vds::certificate_store store;
        store.add(ca_certificate);
        
        auto result = store.verify(caudal_certificate);
        ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
        ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=Sub Cert");
        ASSERT_FALSE(caudal_certificate.is_issued(ca_certificate));
        ASSERT_TRUE(caudal_certificate.is_ca_cert());
        ASSERT_TRUE(ca_certificate.is_ca_cert());
      }
      {
        vds::certificate ca_certificate = vds::certificate::parse(ca_certificate_text);
        vds::certificate sub_certificate = vds::certificate::parse(sub_certificate_text);
        vds::certificate caudal_certificate = vds::certificate::parse(caudal_certificate_text);

        vds::certificate_store store;
        store.add(sub_certificate);
        store.add(ca_certificate);
        
        auto result = store.verify(caudal_certificate);
        ASSERT_EQ(result.error_code, 0);
        ASSERT_TRUE(caudal_certificate.is_issued(sub_certificate));
        ASSERT_TRUE(sub_certificate.is_issued(ca_certificate));
        ASSERT_FALSE(caudal_certificate.is_issued(ca_certificate));
        ASSERT_TRUE(ca_certificate.is_issued(ca_certificate));
        ASSERT_TRUE(caudal_certificate.is_ca_cert());
        ASSERT_TRUE(sub_certificate.is_ca_cert());
        ASSERT_TRUE(ca_certificate.is_ca_cert());
      }
      registrator.shutdown(sp);
    }
    
}

TEST(test_certificates, test_der)
{
  vds::service_registrator registrator;

  vds::crypto_service crypto_service;
  vds::console_logger console_logger(
      test_config::instance().log_level(),
      test_config::instance().modules());

  registrator.add(console_logger);
  registrator.add(crypto_service);
  {
    auto sp = registrator.build("test_der");
    registrator.start(sp);

    const std::string der_password("123qwe");
    //Generate CA certificate
    vds::const_data_buffer ca_certificate_text;
    vds::const_data_buffer ca_private_key;

    {
      vds::asymmetric_private_key ca_certificate_private_key(vds::asymmetric_crypto::rsa2048());
      ca_certificate_private_key.generate();

      vds::asymmetric_public_key ca_certificate_public_key(ca_certificate_private_key);

      //Create CA certificate
      vds::certificate::create_options ca_options;
      ca_options.country = "RU";
      ca_options.organization = "Test Org";
      ca_options.name = "CA Cert";

      vds::certificate ca = vds::certificate::create_new(
        ca_certificate_public_key,
        ca_certificate_private_key,
        ca_options
      );

      ca_certificate_text = ca.der();
      ca_private_key = ca_certificate_private_key.der(sp, der_password);
    }

    //Generate sub certificate
    vds::const_data_buffer sub_certificate_text;
    vds::const_data_buffer sub_private_key;
    
    vds::asymmetric_private_key::parse_der(sp, ca_private_key, der_password)
    .then(
      [&sub_certificate_text, &sub_private_key, ca_certificate_text, der_password](
        const std::function<void (const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::asymmetric_private_key & ca_certificate_private_key) {
      
        vds::certificate ca = vds::certificate::parse_der(ca_certificate_text);

        vds::asymmetric_private_key sub_certificate_private_key(vds::asymmetric_crypto::rsa2048());
        sub_certificate_private_key.generate();

        vds::asymmetric_public_key sub_certificate_public_key(sub_certificate_private_key);

        //Create sub certificate
        vds::certificate::create_options sub_options;
        sub_options.country = "RU";
        sub_options.organization = "Test Org";
        sub_options.name = "Sub Cert";
        sub_options.ca_certificate = &ca;
        sub_options.ca_certificate_private_key = &ca_certificate_private_key;

        vds::certificate sub_certificate = vds::certificate::create_new(
          sub_certificate_public_key,
          sub_certificate_private_key,
          sub_options
        );

        sub_certificate_text = sub_certificate.der();
        sub_private_key = sub_certificate_private_key.der(sp, der_password);
        
        done(sp);
      })
    .wait(
      [](const vds::service_provider & sp){},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex){
        throw *ex;
      },
      sp);
    
    //Generate sub certificate
    vds::const_data_buffer caudal_certificate_text;
    std::string caudal_private_key;
    
    vds::asymmetric_private_key::parse_der(sp, sub_private_key, der_password)
    .then(
      [&caudal_certificate_text, &caudal_private_key, sub_certificate_text](
        const std::function<void (const vds::service_provider & sp)> & done,
        const vds::error_handler & on_error,
        const vds::service_provider & sp,
        const vds::asymmetric_private_key & sub_certificate_private_key) {
        
        vds::certificate sub_certificate = vds::certificate::parse_der(sub_certificate_text);

        vds::asymmetric_private_key caudal_certificate_private_key(vds::asymmetric_crypto::rsa2048());
        caudal_certificate_private_key.generate();

        vds::asymmetric_public_key caudal_certificate_public_key(caudal_certificate_private_key);

        //Create sub certificate
        vds::certificate::create_options caudal_options;
        caudal_options.country = "RU";
        caudal_options.organization = "Test Org";
        caudal_options.name = "Caudal Cert";
        caudal_options.ca_certificate = &sub_certificate;
        caudal_options.ca_certificate_private_key = &sub_certificate_private_key;

        vds::certificate caudal_certificate = vds::certificate::create_new(
          caudal_certificate_public_key,
          caudal_certificate_private_key,
          caudal_options
        );

        caudal_certificate_text = caudal_certificate.der();
        caudal_private_key = caudal_certificate_private_key.str();
        
        done(sp);
      })
    .wait(
      [](const vds::service_provider & sp){},
      [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex){
        throw *ex;
      },
      sp);

    //Check
    {
      vds::certificate caudal_certificate = vds::certificate::parse_der(caudal_certificate_text);

      vds::certificate_store store;

      auto result = store.verify(caudal_certificate);
      ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
      ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=Sub Cert");
      ASSERT_TRUE(caudal_certificate.is_ca_cert());
    }
    {
      vds::certificate sub_certificate = vds::certificate::parse_der(sub_certificate_text);
      vds::certificate caudal_certificate = vds::certificate::parse_der(caudal_certificate_text);

      vds::certificate_store store;
      store.add(sub_certificate);

      auto result = store.verify(caudal_certificate);
      ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT);
      ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=CA Cert");
      ASSERT_TRUE(caudal_certificate.is_issued(sub_certificate));
      ASSERT_TRUE(caudal_certificate.is_ca_cert());
      ASSERT_TRUE(sub_certificate.is_ca_cert());
    }
    {
      vds::certificate ca_certificate = vds::certificate::parse_der(ca_certificate_text);
      vds::certificate caudal_certificate = vds::certificate::parse_der(caudal_certificate_text);

      vds::certificate_store store;
      store.add(ca_certificate);

      auto result = store.verify(caudal_certificate);
      ASSERT_EQ(result.error_code, X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY);
      ASSERT_EQ(result.issuer, "/C=RU/O=Test Org/CN=Sub Cert");
      ASSERT_FALSE(caudal_certificate.is_issued(ca_certificate));
      ASSERT_TRUE(caudal_certificate.is_ca_cert());
      ASSERT_TRUE(ca_certificate.is_ca_cert());
    }
    {
      vds::certificate ca_certificate = vds::certificate::parse_der(ca_certificate_text);
      vds::certificate sub_certificate = vds::certificate::parse_der(sub_certificate_text);
      vds::certificate caudal_certificate = vds::certificate::parse_der(caudal_certificate_text);

      vds::certificate_store store;
      store.add(sub_certificate);
      store.add(ca_certificate);

      auto result = store.verify(caudal_certificate);
      ASSERT_EQ(result.error_code, 0);
      ASSERT_TRUE(caudal_certificate.is_issued(sub_certificate));
      ASSERT_TRUE(sub_certificate.is_issued(ca_certificate));
      ASSERT_FALSE(caudal_certificate.is_issued(ca_certificate));
      ASSERT_TRUE(ca_certificate.is_issued(ca_certificate));
      ASSERT_TRUE(caudal_certificate.is_ca_cert());
      ASSERT_TRUE(sub_certificate.is_ca_cert());
      ASSERT_TRUE(ca_certificate.is_ca_cert());
    }
    registrator.shutdown(sp);
  }

}

