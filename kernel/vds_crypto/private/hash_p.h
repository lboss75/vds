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
    static const hash_info & sha256();

    _hash(const hash_info & info);
    ~_hash();

    void update(
      const void * data,
      size_t len);

    void final();

    const const_data_buffer & signature() const {
      return this->sig_;
    }

  private:
    const hash_info & info_;

    EVP_MD_CTX * ctx_;
    const_data_buffer sig_;
  };

  class _hmac
  {
  public:
    _hmac(const std::string & key, const hash_info & info = hash::sha256());
    ~_hmac();

    void update(
      const void * data,
      size_t len);

    void final();

    const const_data_buffer signature() const {
      return this->sig_;
    }

  private:
    const hash_info & info_;
    HMAC_CTX * ctx_;
    const_data_buffer sig_;
    
#if !defined(_WIN32) && !defined(ANDROID)
    HMAC_CTX hmac_ctx_;
#endif
  };

}

#endif // __VDS_CRYPTO_HASH_P_H_
