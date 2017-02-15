/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_certificates.h"

TEST(test_certificates, test_symmetric)
{
    vds::service_registrator registrator;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(crypto_service);
    {
      auto sp = registrator.build();

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
        caudal_options.name = "Sub Cert";
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

        caudal_certificate.verify();

      }
    }
    registrator.shutdown();
    
}
