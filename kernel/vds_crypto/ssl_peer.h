#ifndef __VDS_CRYPTO_SSL_PEER_H_
#define __VDS_CRYPTO_SSL_PEER_H_
/*
Copyright (c) 2017, Vadim Malyshev, lboss75@gmail.com
All rights reserved
*/

namespace vds {
  class certificate;
  class asymmetric_private_key;

  class ssl_peer {
  public:
    ssl_peer(
      bool is_client,
      const certificate * cert,
      const asymmetric_private_key * key
    );
    
    bool is_client() const {
      return this->is_client_;
    }

    std::function<void(void)> input_done;
    std::function<void(const void * data, size_t len)> decoded_output_done;

    std::function<void(void)> decoded_input_done;
    std::function<void(const void * data, size_t len)> output_done;

    void write_input(const void * data, size_t len);
    void read_decoded_output(void * data, size_t len);

    void read_output(void * data, size_t len);
    void write_decoded_output(const void * data, size_t len);

  private:
    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    bool is_client_;

    const void * input_data_;
    size_t input_len_;

    void * output_data_;
    size_t output_len_;

    const void * decoded_input_data_;
    size_t decoded_input_len_;

    void * decoded_output_data_;
    size_t decoded_output_len_;

    std::mutex data_mutex_;

    void work_circle();
  };

  class ssl_input_stream
  {
  public:
    ssl_input_stream(ssl_peer & peer)
      : peer_(peer)
    {
    }

    template < typename context_type >
    class handler : public sequence_step<context_type, void(const void *, size_t)>

    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_input_stream & args
      ) : base_class(context),
        peer_(args.peer_)
      {
        this->peer_.input_done = [this]() {
          this->prev();
        };

        this->peer_.decoded_output_done = [this](const void * data, size_t len) {
          this->next(data, len);
        };

        this->peer_.read_decoded_output(this->buffer_, BUFFER_SIZE);
      }

      void operator()(const void * data, size_t len)
      {
        if (0 == len) {
          this->next(nullptr, 0);
        }
        else {
          this->peer_.write_input(data, len);
        }
      }

      void processed()
      {
        this->peer_.read_decoded_output(this->buffer_, BUFFER_SIZE);
      }

    private:
      ssl_peer & peer_;
      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
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
    class handler : public sequence_step<context_type, void(const void *, size_t)>
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_output_stream & args
      )
        : base_class(context),
        peer_(args.peer_)
      {
        this->peer_.decoded_input_done = [this]() {
          this->prev();
        };

        this->peer_.output_done = [this](const void * data, size_t len) {
          this->next(data, len);
        };

        this->peer_.read_output(this->buffer_, BUFFER_SIZE);
      }

      void operator()(const void * data, size_t len)
      {
        if (0 == len) {
          this->next(nullptr, 0);
        }
        else {
          this->peer_.write_decoded_output(data, len);
        }
      }

      void processed()
      {
        this->peer_.read_output(this->buffer_, BUFFER_SIZE);
      }

    private:
      ssl_peer & peer_;
      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
    };

  private:
    ssl_peer & peer_;
  };

}

#endif//__VDS_CRYPTO_SSL_PEER_H_
