#ifndef __VDS_CRYPTO_SSL_PEER_H_
#define __VDS_CRYPTO_SSL_PEER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/
namespace vds {

  class ssl_peer {
  public:
    ssl_peer(bool is_client);

  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * read_bio_;
    BIO * write_bio_;

    bool is_client_;
  };

  class ssl_input_stream
  {
  public:
    ssl_input_stream(ssl_peer & peer)
    : peer_(peer)
    {
    }

    template < typename context_type >
    class handler : public sequence_step<context_type, void(void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(void *, size_t)>;
    public:

    };

  private:
    ssl_peer & peer_;
  };

  class ssl_output_stream
  {
  public:
    ssl_output_stream(ssl_peer & peer)
      : peer_(peer)
    {
    }

    template < typename context_type >
    class handler : public sequence_step<context_type, void(void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(void *, size_t)>;
    public:

    };

  private:
    ssl_peer & peer_;
  };
}

#endif//__VDS_CRYPTO_SSL_PEER_H_
