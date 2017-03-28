/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"

template <typename context_type>
class random_reader : public vds::sequence_step<context_type, void(const void *, size_t)>
{
  using base_class = vds::sequence_step<context_type, void(const void *, size_t)>;
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
      auto l = this->len_;

      this->data_ += n;
      this->len_ -= n;

      this->next(p, l);

      break;
    }
  }

private:
  const uint8_t * data_;
  size_t len_;
};

template <typename context_type>
class compare_data : public vds::sequence_step<context_type, void()>
{
  using base_class = vds::sequence_step<context_type, void()>;
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
      auto sp = registrator.build();
      
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

      sequence(
        vds::create_step<random_reader>::with(buffer.get(), (int)len),
        vds::symmetric_encrypt(key),
        vds::symmetric_decrypt(key),
        vds::create_step<compare_data>::with(buffer.get(), (int)len)
      )(
        []() {
      },
        [](std::exception * ex) {
        GTEST_FAIL() << ex->what();
      });
    }
    registrator.shutdown();
    
}

TEST(test_vds_crypto, test_asymmetric)
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
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 128 * 1024;
    } while (len < 8 || len > 128 * 1024);

    vds::data_buffer buffer;
    buffer.resize(len);
    vds::crypto_service::rand_bytes(const_cast<uint8_t *>(buffer.data()), (int)len);

    vds::asymmetric_private_key private_key(vds::asymmetric_crypto::rsa2048());
    private_key.generate();

    vds::asymmetric_public_key public_key(private_key);

    auto result = private_key.decrypt(public_key.encrypt(buffer));

    ASSERT_EQ(result.size(), buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i) {
      ASSERT_EQ(result[i], buffer[i]);
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
      vds::crypto_service::rand_bytes(&len, sizeof(len));
      len %= 1024 * 1024 * 1024;
    } while (len < 1024 || len > 1024 * 1024 * 1024);

    std::unique_ptr<unsigned char> buffer(new unsigned char[len]);
    vds::crypto_service::rand_bytes(buffer.get(), (int)len);

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
