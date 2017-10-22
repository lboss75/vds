/*file:///home/vadim/projects/vds.git/tests/test_vds_core/CMakeLists.txt
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"
#include "random_stream.h"
#include "compare_data.h"
#include "test_config.h"

TEST(test_vds_crypto, test_symmetric)
{
    vds::service_registrator registrator;
    vds::mt_service mt_service;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(
      test_config::instance().log_level(),
      test_config::instance().modules());

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
  vds::console_logger console_logger(
      test_config::instance().log_level(),
      test_config::instance().modules());

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

    vds::asymmetric_sign s(vds::hash::sha256(), key);
    s.write(sp, buffer.get(), len);
    s.write(sp, nullptr, 0);
    
    auto sign = s.signature();
    vds::asymmetric_public_key pkey(key);

    
    vds::asymmetric_sign_verify v(vds::hash::sha256(), pkey, sign);
    v.write(sp, buffer.get(), len);
    v.write(sp, nullptr, 0);
    GTEST_ASSERT_EQ(v.result(), true);
    
    vds::asymmetric_sign_verify sv(vds::hash::sha256(), pkey, sign);
    random_stream<uint8_t> rs(sv);
    rs.write(sp, buffer.get(), len);
    rs.write(sp, nullptr, 0);
    GTEST_ASSERT_EQ(sv.result(), true);

    size_t index;
    do
    {
      vds::crypto_service::rand_bytes(&index, sizeof(index));
      index %= len;
    } while (index >= len);

    const_cast<unsigned char *>(buffer.get())[index]++;

    vds::asymmetric_sign_verify sv1(vds::hash::sha256(), pkey, sign);
    random_stream<uint8_t> rs1(sv1);
    rs1.write(sp, buffer.get(), len);
    rs1.write(sp, nullptr, 0);
    GTEST_ASSERT_EQ(sv1.result(), false);

    registrator.shutdown(sp);
  }
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
