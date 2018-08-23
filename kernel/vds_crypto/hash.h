#ifndef __VDS_CRYPTO_HASH_H_
#define __VDS_CRYPTO_HASH_H_

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

    hash(const hash_info & info);
    ~hash();

    void update(
      const void * data,
      size_t len);

    void final();

    const class const_data_buffer & signature() const;
    
    static const_data_buffer signature(
      const hash_info & info,
      const const_data_buffer & data);

    static const_data_buffer signature(
      const hash_info & info,
      const void * data,
      size_t data_size);
    
  private:
    class _hash * impl_;
  };

  class _hmac;
  class hmac
  {
  public:
    hmac(
        const const_data_buffer & key,
        const hash_info & info = hash::sha256());
    ~hmac();

    void update(
      const void * data,
      size_t len);

    const_data_buffer && final();

    static const const_data_buffer signature(
        const const_data_buffer & key,
        const hash_info & info,
        const void * data,
        size_t len) {
      hmac h(key, info);
      h.update(data, len);
      return h.final();
    }

  private:
    _hmac * impl_;
  };
}

#endif // __HASH_H_
