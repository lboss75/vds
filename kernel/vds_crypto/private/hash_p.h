#ifndef __VDS_CRYPTO_HASH_P_H_
#define __VDS_CRYPTO_HASH_P_H_

/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  struct hash_info
  {
    int id;
    const EVP_MD * type;
  };
  class _hash
  {
  public:
    _hash();
    ~_hash();

    expected<void> create(const hash_info & info);

    expected<void> update(
      const void * data,
      size_t len);

    expected<void> final();

    const const_data_buffer & signature() const {
      vds_assert(this->sig_);
      return this->sig_;
    }

  private:
    const hash_info * info_;

    EVP_MD_CTX * ctx_;
    const_data_buffer sig_;
  };

  class _hmac
  {
  public:
    _hmac(const const_data_buffer & key, const hash_info & info = hash::sha256());
    ~_hmac();

    expected<void> update(
      const void * data,
      size_t len);

    expected<const_data_buffer> final();

  private:
    const hash_info & info_;
    HMAC_CTX * ctx_;
#if OPENSSL_VERSION_NUMBER < 0x1010007fL
    HMAC_CTX ctx_data_;
#endif

  };

}

#endif // __VDS_CRYPTO_HASH_P_H_
