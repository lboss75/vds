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

    bool write_input(const void * data, size_t len);
    size_t read_input(uint8_t * data, size_t len);

    bool write_output(const void * data, size_t len);
    size_t read_output(uint8_t * data, size_t len);

  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * read_bio_;
    BIO * write_bio_;

    bool is_client_;
    bool is_handshaking_;
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
      handler(
        const context_type & context,
        const ssl_input_stream & args
        ) : data_(nullprt)
      {

      }

      void operator()(const void * data, size_t len)
      {
        if (!this->peer_.write_input(data, len)) {
          this->data_ = data;
          this->len_ = len;
        }
        else {
          this->processed();
        }
      }

      void processed()
      {
        auto len = this->peer_.read_input(this->buffer_, BUFFER_SIZE);
        if (0 == len) {
          if (nullptr == this->data_) {
            this->prev();
          }
          else {
            auto p = this->data_;
            auto l = this->len_;

            this->data_ = nullptr;
            this->len_ = 0;

            (*this)(p, l);
          }
        }
        else {
          this->next(this->buffer_, len);
        }
      }
    };

  private:
    ssl_peer & peer_;

    const void * data_;
    size_t len_;

    static constexpr size_t BUFFER_SIZE = 1024;
    uint8_t buffer_[BUFFER_SIZE];
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

      void operator()(const void * data, size_t len)
      {
        this->peer_.write_output(data, len);
        this->processed();
      }

      void processed()
      {
        auto len = this->peer_.read_output(this->buffer_, BUFFER_SIZE);
        if (0 == len) {
          this->prev();
        }
        else {
          this->next(this->buffer_, len);
        }
      }
    };

  private:
    ssl_peer & peer_;
    static constexpr size_t BUFFER_SIZE = 1024;
    uint8_t buffer_[BUFFER_SIZE];
  };
}

#endif//__VDS_CRYPTO_SSL_PEER_H_
