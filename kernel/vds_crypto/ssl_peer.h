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

    certificate get_peer_certificate() const;

  private:
    friend class ssl_input_stream;
    friend class ssl_output_stream;

    class issl_input_stream
    {
    public:
      virtual void input_done() = 0;
      virtual void decoded_output_done(size_t len) = 0;

      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
    };

    class issl_output_stream
    {
    public:
      virtual void decoded_input_done() = 0;
      virtual void output_done(size_t len) = 0;

      static constexpr size_t BUFFER_SIZE = 1024;
      uint8_t buffer_[BUFFER_SIZE];
    };


    SSL_CTX *ssl_ctx_;
    SSL * ssl_;
    BIO * input_bio_;
    BIO * output_bio_;

    bool is_client_;

    const void * input_data_;
    size_t input_len_;

    const void * decoded_input_data_;
    size_t decoded_input_len_;

    issl_input_stream * input_stream_;
    issl_output_stream * output_stream_;

    bool input_stream_done_;
    bool output_stream_done_;

    std::mutex work_circle_mutex_;
    int work_circle_queries_;

    bool enable_output_;

    void set_input_stream(issl_input_stream * stream);
    void set_output_stream(issl_output_stream * stream);

    void write_input(const void * data, size_t len);
    void write_decoded_output(const void * data, size_t len);

    void start_work_circle();
    void work_circle();
    void input_stream_processed();
    void output_stream_processed();
  };

  class ssl_input_stream
  {
  public:
    ssl_input_stream(ssl_peer & peer)
      : peer_(peer)
    {
    }

    template < typename context_type >
    class handler
      : public sequence_step<context_type, void(const void *, size_t)>,
        public ssl_peer::issl_input_stream
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_input_stream & args
      );

      ~handler();

      void input_done() override;
      void decoded_output_done(size_t len) override;

      void operator()(const void * data, size_t len);
      void processed();

    private:
      ssl_peer & peer_;
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
    class handler
      : public sequence_step<context_type, void(const void *, size_t)>,
        public ssl_peer::issl_output_stream
    {
      using base_class = sequence_step<context_type, void(const void *, size_t)>;
    public:
      handler(
        const context_type & context,
        const ssl_output_stream & args
      );

      void decoded_input_done() override;
      void output_done(size_t len) override;

      void operator()(const void * data, size_t len);
      void processed();

    private:
      ssl_peer & peer_;
    };

  private:
    ssl_peer & peer_;
  };

  template<typename context_type>
  inline ssl_input_stream::handler<context_type>::handler(const context_type & context, const ssl_input_stream & args)
  : base_class(context),
    peer_(args.peer_)
  {
    this->peer_.set_input_stream(this);
  }

  template<typename context_type>
  inline ssl_input_stream::handler<context_type>::~handler()
  {
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::input_done()
  {
    this->prev();
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::decoded_output_done(size_t len)
  {
    this->next(this->buffer_, len);
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::operator()(const void * data, size_t len)
  {
    if (0 == len) {
      this->next(nullptr, 0);
    }
    else {
      this->peer_.write_input(data, len);
    }
  }

  template<typename context_type>
  inline void ssl_input_stream::handler<context_type>::processed()
  {
    this->peer_.input_stream_processed();
  }


  template<typename context_type>
  inline ssl_output_stream::handler<context_type>::handler(const context_type & context, const ssl_output_stream & args)
  : base_class(context),
    peer_(args.peer_)
  {
    this->peer_.set_output_stream(this);
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::decoded_input_done()
  {
    this->prev();
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::output_done(size_t len)
  {
    this->next(this->buffer_, len);
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::operator()(const void * data, size_t len)
  {
    if (0 == len) {
      this->next(nullptr, 0);
    }
    else {
      this->peer_.write_decoded_output(data, len);
    }
  }

  template<typename context_type>
  inline void ssl_output_stream::handler<context_type>::processed()
  {
    this->peer_.output_stream_processed();
  }
}

#endif//__VDS_CRYPTO_SSL_PEER_H_
