#ifndef __VDS_CRYPTO_SSL_TUNNEL_H_
#define __VDS_CRYPTO_SSL_TUNNEL_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "async_stream.h"

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class _ssl_tunnel;
  class ssl_tunnel {
  public:
    ssl_tunnel(
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );

    ~ssl_tunnel();
    
    bool is_client() const;

    std::shared_ptr<continuous_stream<uint8_t>> crypted_input();
    std::shared_ptr<continuous_stream<uint8_t>> crypted_output();

    std::shared_ptr<continuous_stream<uint8_t>> decrypted_input();
    std::shared_ptr<continuous_stream<uint8_t>> decrypted_output();

    void start(const service_provider & sp);
    certificate get_peer_certificate() const;

    void on_error(const std::function<void(const std::shared_ptr<std::exception> &)> & handler);

  private:
    _ssl_tunnel * const impl_;
  };
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
