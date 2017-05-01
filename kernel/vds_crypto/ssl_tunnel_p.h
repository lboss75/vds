#ifndef __VDS_CRYPTO_SSL_TUNNEL_P_H_
#define __VDS_CRYPTO_SSL_TUNNEL_P_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

#include "ssl_tunnel.h"

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class _ssl_tunnel {
  public:
    _ssl_tunnel(
      const service_provider & scope,
      ssl_tunnel * owner,
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );

    ~_ssl_tunnel();
    
    bool is_client() const {
      return this->is_client_;
    }

    certificate get_peer_certificate() const;

  private:
    lifetime_check lt_;
    friend class ssl_input_stream;
    friend class ssl_output_stream;
    friend class ssl_tunnel;

    service_provider sp_;
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    ssl_tunnel * owner_;
    bool is_client_;

    const void * input_data_;
    size_t input_len_;

    const void * decoded_input_data_;
    size_t decoded_input_len_;

    ssl_tunnel::issl_input_stream * input_stream_;
    ssl_tunnel::issl_output_stream * output_stream_;

    bool input_stream_done_;
    bool output_stream_done_;

    std::mutex work_circle_mutex_;
    int work_circle_queries_;

    bool enable_output_;

    void set_input_stream(ssl_tunnel::issl_input_stream * stream);
    void set_output_stream(ssl_tunnel::issl_output_stream * stream);

    void write_input(const void * data, size_t len);
    void write_decoded_output(const void * data, size_t len);

    void start_work_circle();
    void work_circle();
    void input_stream_processed(const service_provider & sp);
    void output_stream_processed(const service_provider & sp);
  };
}

#endif//__VDS_CRYPTO_SSL_TUNNEL_H_
