/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"

template <typename context_type>
class random_reader : public vds::dataflow_step<context_type, bool(const void *, size_t)>
{
  using base_class = vds::dataflow_step<context_type, bool(const void *, size_t)>;
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

  bool operator()(const vds::service_provider & sp)
  {
    return this->continue_process(sp);
  }

  void processed(const vds::service_provider & sp)
  {
    if(this->continue_process(sp)){
      this->prev(sp);
    }    
  }
  
private:
  bool continue_process(const vds::service_provider & sp)
  {
    for(;;){
      if (0 == this->len_) {
        if(this->next(sp, nullptr, 0)){
          continue;
        }
        return false;
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

        if(this->next(sp, p, n)){
          break;
        }

        return false;
      }
    }
  }

  const uint8_t * data_;
  size_t len_;
};

template <typename context_type>
class compare_data : public vds::dataflow_step<context_type, bool(void)>
{
  using base_class = vds::dataflow_step<context_type, bool(void)>;
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

  bool operator()(const vds::service_provider & sp, const void * data, size_t len)
  {
    if (0 == len) {
      if(0 != this->len_){
        throw std::runtime_error("compare_data error");
      }
      return this->next(sp);
    }

    const uint8_t * p = reinterpret_cast<const uint8_t *>(data);
    while (0 < len) {
      auto l = len;
      if (l > this->len_) {
        l = this->len_;
      }
      
      if(0 != memcmp(this->data_, p, l)){
        throw std::runtime_error("compare_data error");
      }

      p += l;
      len -= l;

      this->data_ += l;
      this->len_ -= l;
    }
    
    return false;
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
        [](const vds::service_provider &) {},
        [](const vds::service_provider &, std::exception_ptr ex) {
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
        [&sign](const vds::service_provider & sp, const void * data, size_t len) { sign.reset(data, len); },
        [](const vds::service_provider & sp, std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);


    vds::asymmetric_public_key pkey(key);

    
    bool unchanged_result;
    vds::dataflow(
      vds::create_step<random_reader>::with(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [&unchanged_result](const vds::service_provider & sp, bool result) { unchanged_result = result; },
        [](const vds::service_provider & sp, std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
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
        [&changed_result](const vds::service_provider & sp, bool result) { changed_result = result; },
        [](const vds::service_provider & sp, std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
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
