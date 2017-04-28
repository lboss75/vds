/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"

template <typename context_type>
class random_reader : public vds::dataflow_step<context_type, void(const void *, size_t)>
{
  using base_class = vds::dataflow_step<context_type, void(const void *, size_t)>;
public:
  random_reader(
    const context_type & context,
    const void * data,
    size_t len)
    : base_class(context),
    data_(reinterpret_cast<const uint8_t *>(data)),
    len_(len)
  {
  }

  void operator()()
  {
    this->processed();
  }

  void processed()
  {
    if (0 == this->len_) {
      this->next(nullptr, 0);
      return;
    }

    for(;;){
      size_t n = (size_t)std::rand();
      if (n < 1) {
        continue;
      }

      if (n > this->len_) {
        n = this->len_;
      }

      auto p = this->data_;

      this->data_ += n;
      this->len_ -= n;

      this->next(p, n);

      break;
    }
  }

private:
  const uint8_t * data_;
  size_t len_;
};

template <typename context_type>
class compare_data : public vds::dataflow_step<context_type, void()>
{
  using base_class = vds::dataflow_step<context_type, void()>;
public:
  compare_data(
    const context_type & context,
    const void * data,
    size_t len)
    : base_class(context),
    data_(reinterpret_cast<const uint8_t *>(data)),
    len_(len)
  {
  }

  void operator()(const void * data, size_t len)
  {
    if (0 == len) {
      ASSERT_EQ(this->len_, 0);
      this->next();
    }

    const uint8_t * p = reinterpret_cast<const uint8_t *>(data);
    while (0 < len) {
      auto l = len;
      if (l > this->len_) {
        l = this->len_;
      }
      ASSERT_EQ(memcmp(this->data_, p, l), 0);

      p += l;
      len -= l;

      this->data_ += l;
      this->len_ -= l;
    }
  }

private:
  const uint8_t * data_;
  size_t len_;
};

TEST(test_vds_crypto, test_symmetric)
{
    vds::service_registrator registrator;
    
    vds::crypto_service crypto_service;
    vds::console_logger console_logger(vds::ll_trace);

    registrator.add(console_logger);
    registrator.add(crypto_service);
    {
      auto sp = registrator.build("test_symmetric");
      
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

      dataflow(
        vds::create_step<random_reader>::with(buffer.get(), (int)len),
        vds::symmetric_encrypt(key),
        vds::symmetric_decrypt(key),
        vds::create_step<compare_data>::with(buffer.get(), (int)len)
      )(
        []() {},
        [](std::exception_ptr ex) {
          GTEST_FAIL() << vds::exception_what(ex);
        },
        sp);

      registrator.shutdown(sp);
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

  vds::crypto_service crypto_service;
  vds::console_logger console_logger(vds::ll_trace);

  registrator.add(console_logger);
  registrator.add(crypto_service);
  {
    auto sp = registrator.build("test_sign");
    
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
      vds::create_step<random_reader>::with(buffer.get(), (int)len),
      vds::asymmetric_sign(vds::hash::sha256(), key))
      (
        [&sign](const void * data, size_t len) { sign.reset(data, len); },
        [](std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);


    vds::asymmetric_public_key pkey(key);

    
    bool unchanged_result;
    vds::dataflow(
      vds::create_step<random_reader>::with(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [&unchanged_result](bool result) { unchanged_result = result; },
        [](std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);

    size_t index;
    do
    {
      vds::crypto_service::rand_bytes(&index, sizeof(index));
      index %= len;
    } while (index >= len);

    const_cast<unsigned char *>(buffer.get())[index]++;

    bool changed_result;
    vds::dataflow(
      vds::create_step<random_reader>::with(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [&changed_result](bool result) { changed_result = result; },
        [](std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);

    ASSERT_EQ(unchanged_result, true);
    ASSERT_EQ(changed_result, false);

    registrator.shutdown(sp);
  }
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
