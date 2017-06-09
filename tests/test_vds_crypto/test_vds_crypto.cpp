/*file:///home/vadim/projects/vds.git/tests/test_vds_core/CMakeLists.txt
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"
#include "random_reader.h"
#include "compare_data.h"

TEST(test_vds_crypto, test_symmetric)
{
    vds::service_registrator registrator;
    vds::mt_service mt_service;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(mt_service);
    registrator.add(console_logger);
    registrator.add(crypto_service);
    {
      auto sp = registrator.build("test_symmetric");
      registrator.start(sp);
      
      size_t len;
      do
      {
        vds::crypto_service::rand_bytes(&len, sizeof(len));
        len %= 32 * 1024 * 1024;
      }while(len < 1024 || len > 32 * 1024 * 1024);

      std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
      vds::crypto_service::rand_bytes(buffer.get(), (int)len);

      vds::symmetric_key key(vds::symmetric_crypto::aes_256_cbc());
      key.generate();

      vds::barrier b;
      const std::shared_ptr<std::exception> & error;
      dataflow(
        random_reader<uint8_t>(buffer.get(), (int)len),
        vds::symmetric_encrypt(key),
        vds::symmetric_decrypt(key),
        compare_data<uint8_t>(buffer.get(), (int)len)
      )(
        [&b](const vds::service_provider &) {b.set(); },
        [&b, &error](const vds::service_provider &, const std::shared_ptr<std::exception> & ex) {
          error = ex;
          b.set();
        },
        sp);

      b.wait();
      registrator.shutdown(sp);
      if (error) {
        GTEST_FAIL() << vds::exception_what(error);
      }
    }
}

TEST(test_vds_crypto, test_asymmetric)
{
  vds::service_registrator registrator;

  vds::crypto_service crypto_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(crypto_service);
  {
    auto sp = registrator.build("test_asymmetric");

    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 128 * 1024;
    } while (len < 8 || len > 128 * 1024);

    std::vector<uint8_t> buffer(len);
    vds::crypto_service::rand_bytes(buffer.data(), (int)len);

    vds::asymmetric_private_key private_key(vds::asymmetric_crypto::rsa2048());
    private_key.generate();

    vds::asymmetric_public_key public_key(private_key);

    auto result = private_key.decrypt(public_key.encrypt(buffer.data(), buffer.size()));

    ASSERT_EQ(result.size(), buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
      ASSERT_EQ(result[i], buffer[i]);
    }

    registrator.shutdown(sp);
  }
}


TEST(test_vds_crypto, test_sign)
{
  vds::service_registrator registrator;
  vds::mt_service mt_service;

  vds::crypto_service crypto_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(mt_service);
  registrator.add(console_logger);
  registrator.add(crypto_service);
  {
    auto sp = registrator.build("test_sign");
    registrator.start(sp);
    
    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 16 * 1024 * 1024;
    } while (len < 1024 || len > 16 * 1024 * 1024);

    std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
    vds::crypto_service::rand_bytes(buffer.get(), (int)len);

    vds::asymmetric_private_key key(vds::asymmetric_crypto::rsa2048());
    key.generate();

    vds::const_data_buffer sign;
    vds::dataflow(
      random_reader<uint8_t>(buffer.get(), (int)len),
      vds::asymmetric_sign(vds::hash::sha256(), key, sign))
      (
        [&sign](const vds::service_provider & sp) { },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { FAIL() << ex->what(); },
        sp);


    vds::asymmetric_public_key pkey(key);

    
    vds::dataflow(
      random_reader<uint8_t>(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [](const vds::service_provider & sp) { },
        [](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { FAIL() << ex->what(); },
        sp);

    size_t index;
    do
    {
      vds::crypto_service::rand_bytes(&index, sizeof(index));
      index %= len;
    } while (index >= len);

    const_cast<unsigned char *>(buffer.get())[index]++;

    vds::barrier b;
    const std::shared_ptr<std::exception> & error;
    vds::dataflow(
      random_reader<uint8_t>(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [&b](const vds::service_provider & sp) { b.set(); },
        [&b, &error](const vds::service_provider & sp, const std::shared_ptr<std::exception> & ex) { error = ex; b.set(); },
        sp);

    b.wait();

    registrator.shutdown(sp);

    if (!error) {
      GTEST_FAIL() << "Sign failed";
    }

  }
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
