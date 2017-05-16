/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "stdafx.h"
#include "test_vds_crypto.h"

class random_reader
{
public:
  random_reader(
    const void * data,
    size_t len)
  : data_(reinterpret_cast<const uint8_t *>(data)),
    len_(len)
  {
  }

  using outgoing_item_type = uint8_t;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_source<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_source<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const random_reader & args)
      : base_class(context),
      data_(args.data_),
      len_(args.len_)
    {
    }

    size_t sync_get_data(
      const vds::service_provider & sp,
      uint8_t * buffer,
      size_t buffer_size)
    {
      for (;;) {
        size_t n = (size_t)std::rand();
        if (n < 1 || n > buffer_size) {
          continue;
        }

        if (n > this->len_) {
          n = this->len_;
        }

        memcpy(buffer, this->data_, n);

        this->data_ += n;
        this->len_ -= n;

        return n;
      }
    }

  private:
    const uint8_t * data_;
    size_t len_;
  };

private:
  const uint8_t * data_;
  size_t len_;
};

class compare_data
{
public:
  compare_data(
    const void * data,
    size_t len)
    : data_(reinterpret_cast<const uint8_t *>(data)),
    len_(len)
  {
  }

  using incoming_item_type = uint8_t;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t MIN_BUFFER_SIZE = 1;

  template<typename context_type>
  class handler : public vds::sync_dataflow_target<context_type, handler<context_type>>
  {
    using base_class = vds::sync_dataflow_target<context_type, handler<context_type>>;
  public:
    handler(
      const context_type & context,
      const compare_data & args)
      : base_class(context),
      data_(args.data_),
      len_(args.len_)
    {
    }

    size_t sync_push_data(
      const vds::service_provider & sp)
    {
      if (0 == this->input_buffer_size_) {
        if (0 != this->len_) {
          this->on_error(sp, std::make_exception_ptr(std::runtime_error("compare_data error")));
        }
        return 0;
      }

      if (this->len_ < this->input_buffer_size_) {
        this->on_error(sp, std::make_exception_ptr(std::runtime_error("compare_data error")));
        return 0;
      }

      if (0 != memcmp(this->data_, this->input_buffer_, this->input_buffer_size_)) {
        this->on_error(sp, std::make_exception_ptr(std::runtime_error("compare_data error")));
      }

      this->data_ += this->input_buffer_size_;
      this->len_ -= this->input_buffer_size_;

      return this->input_buffer_size_;
    }

  private:
    const uint8_t * data_;
    size_t len_;
  };
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
        random_reader(buffer.get(), (int)len),
        vds::symmetric_encrypt(key),
        vds::symmetric_decrypt(key),
        compare_data(buffer.get(), (int)len)
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
      random_reader(buffer.get(), (int)len),
      vds::asymmetric_sign(vds::hash::sha256(), key, sign))
      (
        [&sign](const vds::service_provider & sp) { },
        [](const vds::service_provider & sp, std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);


    vds::asymmetric_public_key pkey(key);

    
    vds::dataflow(
      random_reader(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [](const vds::service_provider & sp) { },
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
      random_reader(buffer.get(), (int)len),
      vds::asymmetric_sign_verify(vds::hash::sha256(), pkey, sign))
      (
        [](const vds::service_provider & sp) { },
        [](const vds::service_provider & sp, std::exception_ptr ex) { FAIL() << vds::exception_what(ex); },
        sp);

    registrator.shutdown(sp);
  }
}

int main(int argc, char **argv) {
   std::srand(unsigned(std::time(0)));
   setlocale(LC_ALL, "Russian");
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}
