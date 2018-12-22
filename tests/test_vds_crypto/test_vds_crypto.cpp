/*file:///home/vadim/projects/vds.git/tests/test_vds_core/CMakeLists.txt
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"
#include "random_stream.h"
#include "compare_data.h"
#include "test_config.h"

TEST(test_vds_crypto, test_assymmetric)
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
    auto sp = registrator.build();

    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 128 * 1024;
    } while (len < 8 || len > 128 * 1024);

    std::vector<uint8_t> buffer(len);
    vds::crypto_service::rand_bytes(buffer.data(), (int)len);

    auto private_key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa2048());

    vds::asymmetric_public_key public_key(private_key);

    auto result = private_key.decrypt(public_key.encrypt(buffer.data(), buffer.size()));

    ASSERT_EQ(result.size(), buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
      ASSERT_EQ(result[i], buffer[i]);
    }

    registrator.shutdown();
  }
}

static const uint8_t pack_block_iv[] = {
  // 0     1     2     3     4     5     6     7
  0xa5, 0xbb, 0x9f, 0xce, 0xc2, 0xe4, 0x4b, 0x91,
  0xa8, 0xc9, 0x59, 0x44, 0x62, 0x55, 0x90, 0x24
};

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
    auto sp = registrator.build();

    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 128 * 1024;
    } while (len < 8 || len > 128 * 1024);

    std::vector<uint8_t> buffer(len);
    vds::crypto_service::rand_bytes(buffer.data(), (int)len);

    vds::const_data_buffer data(buffer.data(), (int)len);

    std::vector<uint8_t> key_data(vds::symmetric_crypto::aes_256_cbc().key_size());
    vds::crypto_service::rand_bytes(key_data.data(), (int)vds::symmetric_crypto::aes_256_cbc().key_size());
    auto key = vds::symmetric_key::create(
      vds::symmetric_crypto::aes_256_cbc(),
      key_data.data(),
      pack_block_iv);

    auto result1 = vds::symmetric_encrypt::encrypt(key, data);

    auto cmp_stream = std::make_shared<compare_data_async<uint8_t>>(result1.data(), result1.size());
    auto crypto_stream = std::make_shared<vds::symmetric_encrypt>(key, cmp_stream);
    auto rand_stream = std::make_shared<random_stream<uint8_t>>(crypto_stream);

    rand_stream->write_async(buffer.data(), (int)len).get();
    rand_stream->write_async(nullptr, 0).get();
    
    registrator.shutdown();
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
  auto sp = registrator.build();
  try
  {
    registrator.start();
    
    size_t len;
    do
    {
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 16 * 1024 * 1024;
    } while (len < 1024 || len > 16 * 1024 * 1024);

    std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
    vds::crypto_service::rand_bytes(buffer.get(), (int)len);

    auto key = vds::asymmetric_private_key::generate(vds::asymmetric_crypto::rsa2048());

    auto s = std::make_shared<vds::asymmetric_sign>(vds::hash::sha256(), key);
    s->write_async(buffer.get(), len).get();
    s->write_async(nullptr, 0).get();
    
    auto sign = s->signature();
    vds::asymmetric_public_key pkey(key);

    
    auto v = std::make_shared<vds::asymmetric_sign_verify>(vds::hash::sha256(), pkey, sign);
    v->write_async(buffer.get(), len).get();
    v->write_async(nullptr, 0).get();
    GTEST_ASSERT_EQ(v->result(), true);
    
    auto sv = std::make_shared<vds::asymmetric_sign_verify>(vds::hash::sha256(), pkey, sign);
    auto rs = std::make_shared<random_stream<uint8_t>>(sv);
    rs->write_async(buffer.get(), len).get();
    rs->write_async(nullptr, 0).get();
    GTEST_ASSERT_EQ(sv->result(), true);

    size_t index;
    do
    {
      vds::crypto_service::rand_bytes(&index, sizeof(index));
      index %= len;
    } while (index >= len);

    const_cast<unsigned char *>(buffer.get())[index]++;

    auto sv1 = std::make_shared<vds::asymmetric_sign_verify>(vds::hash::sha256(), pkey, sign);
    auto rs1 = std::make_shared<random_stream<uint8_t>>(sv1);
    rs1->write_async(buffer.get(), len).get();
    rs1->write_async(nullptr, 0).get();
    GTEST_ASSERT_EQ(sv1->result(), false);

  } catch(const std::exception & ex){
    try { registrator.shutdown(); } catch (...){}

    GTEST_FAIL() << ex.what();

  } catch(...){
    try { registrator.shutdown(); } catch (...){}
    GTEST_FAIL() << "Unknown error";
  }

  registrator.shutdown();
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
