#ifndef __VDS_CRYPTO_HASH_H_
#define __VDS_CRYPTO_HASH_H_
#include "stream.h"

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class hash
  {
  public:
    static const struct hash_info & md5();
    static const hash_info & sha256();
	static const hash_info & sha1();

    hash();
    hash(const hash & original) = delete;
    hash(hash && original) noexcept;
    ~hash();

    static expected<hash> create(const hash_info & info);

    expected<void> update(
      const void * data,
      size_t len);

    expected<void> final();

    const class const_data_buffer & signature() const;

    static expected<const_data_buffer> signature(
      const hash_info & info,
      expected<const_data_buffer> && data);

    static expected<const_data_buffer> signature(
      const hash_info & info,
      const const_data_buffer & data);

    static expected<const_data_buffer> signature(
      const hash_info & info,
      const void * data,
      size_t data_size);

    hash & operator = (const hash &) = delete;
    hash & operator = (hash && original) noexcept;
    
  private:
    class _hash * impl_;

    hash(_hash * impl)
      : impl_(impl) {      
    }
  };

  class hash_stream_output_async : public stream_output_async<uint8_t> {
  public:
    hash_stream_output_async();
    hash_stream_output_async(const hash_stream_output_async &) = delete;
    hash_stream_output_async(hash_stream_output_async &&) = default;
    hash_stream_output_async(
      hash && hash,
      std::shared_ptr<stream_output_async<uint8_t>> && target);

    static expected<std::shared_ptr<hash_stream_output_async>> create(
      const hash_info & info,
      std::shared_ptr<stream_output_async<uint8_t>> && target);

    async_task<expected<void>> write_async(
      const uint8_t *data,
      size_t len) override;

    const_data_buffer signature() const {
      return this->hash_.signature();
    }

    std::shared_ptr<stream_output_async<uint8_t>> & target() {
      return this->target_;
    }

    const std::shared_ptr<stream_output_async<uint8_t>> & target() const {
      return this->target_;
    }

    hash_stream_output_async& operator = (hash_stream_output_async &&) = default;

  private:
    hash hash_;
    std::shared_ptr<stream_output_async<uint8_t>> target_;
  };


  class _hmac;
  class hmac
  {
  public:
    hmac(
        const const_data_buffer & key,
        const hash_info & info = hash::sha256());
    ~hmac();

    expected<void> update(
      const void * data,
      size_t len);

    expected<const_data_buffer> final();

    static expected<const_data_buffer> signature(
        const const_data_buffer & key,
        const hash_info & info,
        const void * data,
        size_t len) {
      hmac h(key, info);
      CHECK_EXPECTED(h.update(data, len));
      return h.final();
    }

    static expected<bool> verify(
      const const_data_buffer & key,
      const hash_info & info,
      const void * data,
      size_t len,
      const void * signature,
      size_t signature_len) {

      hmac h(key, info);
      CHECK_EXPECTED(h.update(data, len));
      GET_EXPECTED(result, h.final());
      return (result.size() == signature_len)
      && (memcmp(result.data(), signature, signature_len) == 0);
    }

  private:
    _hmac * impl_;
  };
}

#endif // __HASH_H_
