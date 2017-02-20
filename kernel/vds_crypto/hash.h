#ifndef __VDS_CRYPTO_HASH_H_
#define __VDS_CRYPTO_HASH_H_

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

  class hash
  {
  public:
    static const hash_info & sha256();

    hash(const hash_info & info);
    ~hash();

    void update(
      const void * data,
      size_t len);

    void final();

    const unsigned char * signature() const {
      return this->sig_;
    }

    unsigned int signature_length() const {
      return this->sig_len_;
    }

  private:
    const hash_info & info_;

    EVP_MD_CTX * ctx_;
    unsigned char * sig_;
    unsigned int sig_len_;
  };

  class hmac
  {
  public:
    hmac(const std::string & key, const hash_info & info = hash::sha256());
    ~hmac();

    void update(
      const void * data,
      size_t len);

    void final();

    const unsigned char * signature() const {
      return this->sig_;
    }

    unsigned int signature_length() const {
      return this->sig_len_;
    }

  private:
    const hash_info & info_;
    HMAC_CTX * ctx_;
    unsigned char * sig_;
    unsigned int sig_len_;
    
#ifndef _WIN32
    HMAC_CTX hmac_ctx_;
#endif
  };

}

#endif // __HASH_H_
