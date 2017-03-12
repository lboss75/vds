/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"

TEST(test_vds_crypto, test_symmetric)
{
    vds::service_registrator registrator;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(crypto_service);
    {
      auto sp = registrator.build();
      
      size_t len;
      do
      {
        RAND_bytes((unsigned char *)&len, sizeof(len));
        len %= 32 * 1024 * 1024;
      }while(len < 1024 || len > 32 * 1024 * 1024);

      std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
      RAND_bytes(buffer.get(), (int)len);

      vds::symmetric_key key(vds::symmetric_crypto::aes_256_cbc());
      key.generate();

      std::vector<unsigned char> result(len);
      vds::symmetric_encrypt encrypt(key);

      std::vector<unsigned char> decypted(len);
      vds::symmetric_decrypt decrypt(key);
      
      auto p = buffer.get();
      auto l = len;
      
      size_t result_len = 0;
      while(l > 0){
        size_t n = (size_t)std::rand();
        if (n < 1) {
          continue;
        }
        if(n > l) {
          n = l;
        }
        result_len += encrypt.update(p, n, result.data() + result_len, result.size() - result_len);
        p += n;
        l -= n;
      }
      result_len += encrypt.update(nullptr, 0, result.data() + result_len, result.size() - result_len);
      
      auto decypted_len = decrypt.update(result.data(), result_len, decypted.data(), decypted.size());
      decypted_len += decrypt.update(nullptr, 0, decypted.data() + decypted_len, decypted.size() - decypted_len);
    
      ASSERT_EQ(decypted_len, len);
      for(size_t i = 0; i < len; ++i) {
        ASSERT_EQ(decypted[i], buffer.get()[i]);
      }
    }
    registrator.shutdown();
    
}

TEST(test_vds_crypto, test_sign)
{
  vds::service_registrator registrator;

  vds::crypto_service crypto_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(crypto_service);
  {
    auto sp = registrator.build();
    
    size_t len;
    do
    {
      RAND_bytes((unsigned char *)&len, sizeof(len));
      len %= 1024 * 1024 * 1024;
    } while (len < 1024 || len > 1024 * 1024 * 1024);

    std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
    RAND_bytes(buffer.get(), (int)len);

    vds::asymmetric_private_key key(vds::asymmetric_crypto::rsa2048());
    key.generate();

    vds::asymmetric_sign sign(vds::hash::sha256(), key);

    vds::asymmetric_public_key pkey(key);
    vds::asymmetric_sign_verify sverify(vds::hash::sha256(), pkey);

    sign.update(buffer.get(), len);
    sign.final();

    auto p = buffer.get();
    auto l = len;
    while(l > 0) {
      size_t n = (size_t)std::rand();
      if (n < 1) {
        continue;
      }
      if(n > l) {
        n = l;
      }
      sverify.update(p, n);
      p += n;
      l -= n;
    }

    auto result = sverify.verify(sign.signature());

    ASSERT_EQ(result, true);
  }

  registrator.shutdown();
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
